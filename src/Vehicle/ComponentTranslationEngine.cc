#include "ComponentTranslationEngine.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QSet>
#include <QTemporaryFile>
#include <QUrl>
#include <QVector>
#include <QXmlStreamReader>
#include <cmath>

namespace {
constexpr qint64 byteLimit = 32 * 1024 * 1024;
constexpr int chunkSize = 64 * 1024;
constexpr int nodeLimit = 262144;
constexpr int stringLimit = 65536;
constexpr int keyLimit = 128;
constexpr int pathLimit = 16384;
constexpr qint64 jsonCharacters = 16 * 1024 * 1024;
constexpr qint64 xmlCharacters = 8 * 1024 * 1024;
constexpr qint64 replacementCharacters = 16 * 1024 * 1024;
constexpr int dictionaryLimit = 65536;

struct Budget {
    const std::atomic_bool& cancelled;
    const QElapsedTimer& clock;
    QString& error;
    qint64 work = 0;

    bool fail(const char* code) { if (error.isEmpty()) error = QString::fromLatin1(code); return false; }
    bool step(qint64 count = 1) {
        if (cancelled.load(std::memory_order_relaxed)) return fail("translation-cancelled");
        if (!clock.isValid()) return fail("translation-clock-invalid");
        const qint64 elapsed = clock.elapsed();
        if (elapsed < 0 || elapsed >= 120000) return fail("translation-deadline-exceeded");
        if (count < 0 || work > 2000000 - count) return fail("translation-work-limit");
        work += count;
        return true;
    }
};

bool readFile(const QString& path, QByteArray& bytes, Budget& budget)
{
    bytes.clear();
    if (!budget.step() || path.isEmpty() || path.size() > pathLimit || path.contains(QChar(0)))
        return budget.fail("translation-input-path-invalid");
    const bool resource = path.startsWith(QStringLiteral(":/"));
    const QFileInfo before(path);
    if (!before.exists() || !before.isFile() || before.isSymLink() || !before.isReadable()
        || before.size() < 0 || before.size() > byteLimit)
        return budget.fail("translation-input-file-invalid");
    // QFile resources are deliberately supported. Their canonical name need
    // not be a native absolute filesystem path.
    const QString canonical = before.canonicalFilePath();
    if (!resource && canonical.isEmpty()) return budget.fail("translation-input-identity-invalid");
    QFile file(path);
    if (!budget.step() || !file.open(QIODevice::ReadOnly)) return budget.fail("translation-input-open-failed");
    if (!budget.step() || file.size() != before.size()) return budget.fail("translation-input-size-changed");
    while (!file.atEnd()) {
        if (!budget.step()) return false;
        const QByteArray part = file.read(chunkSize);
        if (!budget.step() || file.error() != QFileDevice::NoError || part.isEmpty())
            return budget.fail("translation-input-read-failed");
        if (bytes.size() > byteLimit - part.size()) return budget.fail("translation-input-byte-limit");
        bytes.append(part);
    }
    if (!budget.step() || file.error() != QFileDevice::NoError || !file.atEnd()
        || bytes.size() != before.size() || file.size() != before.size())
        return budget.fail("translation-input-incomplete");
    file.close();
    if (!budget.step() || file.isOpen() || file.error() != QFileDevice::NoError)
        return budget.fail("translation-input-close-failed");
    const QFileInfo after(path);
    if (!after.exists() || !after.isFile() || after.isSymLink() || after.size() != before.size()
        || after.lastModified() != before.lastModified()
        || after.canonicalFilePath() != canonical)
        return budget.fail("translation-input-changed");
    return budget.step();
}

struct Node {
    QJsonValue::Type type = QJsonValue::Null;
    QJsonValue scalar;
    QMap<QString, int> object;
    QVector<int> array;
};

// Physical node numbers are independent of translation context strings and of
// list.key values. Repeated keys must not turn productive recursion into a cycle.
bool flatten(const QJsonValue& root, QVector<Node>& nodes, Budget& budget)
{
    struct Pending { QJsonValue value; int parent; QString key; int index; int depth; };
    QVector<Pending> pending;
    pending.append({root, -1, {}, -1, 1});
    qint64 characters = 0;
    while (!pending.isEmpty()) {
        if (!budget.step()) return false;
        const Pending next = pending.takeLast();
        if (next.depth > 64 || nodes.size() >= nodeLimit) return budget.fail("translation-json-tree-limit");
        Node node;
        node.type = next.value.type();
        if (next.value.isString()) {
            const QString value = next.value.toString();
            if (value.size() > stringLimit || characters > jsonCharacters - value.size())
                return budget.fail("translation-json-text-limit");
            characters += value.size();
        } else if (next.value.isDouble() && !std::isfinite(next.value.toDouble())) {
            return budget.fail("translation-json-number-invalid");
        }
        if (!next.value.isObject() && !next.value.isArray()) node.scalar = next.value;
        const int id = nodes.size();
        nodes.append(node);
        if (next.parent >= 0) {
            if (next.index >= 0) nodes[next.parent].array[next.index] = id;
            else nodes[next.parent].object.insert(next.key, id);
        }
        if (next.value.isObject()) {
            const QJsonObject object = next.value.toObject();
            if (object.size() > dictionaryLimit || pending.size() > nodeLimit - object.size())
                return budget.fail("translation-json-tree-limit");
            for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
                if (!budget.step()) return false;
                if (it.key().size() > keyLimit || characters > jsonCharacters - it.key().size())
                    return budget.fail("translation-json-key-limit");
                characters += it.key().size();
                pending.append({it.value(), id, it.key(), -1, next.depth + 1});
            }
        } else if (next.value.isArray()) {
            const QJsonArray array = next.value.toArray();
            if (array.size() > nodeLimit || pending.size() > nodeLimit - array.size())
                return budget.fail("translation-json-tree-limit");
            nodes[id].array.resize(array.size());
            for (int i = array.size() - 1; i >= 0; --i) {
                if (!budget.step()) return false;
                pending.append({array.at(i), id, {}, i, next.depth + 1});
            }
        }
    }
    return budget.step();
}

bool parseJson(const QByteArray& bytes, QJsonDocument& document, QVector<Node>& nodes, Budget& budget)
{
    if (!budget.step()) return false;
    QJsonParseError parseError;
    document = QJsonDocument::fromJson(bytes, &parseError);
    if (!budget.step()) return false;
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return budget.fail("translation-json-invalid");
    return flatten(document.object(), nodes, budget);
}

bool addXmlText(const QString& text, qint64& total, Budget& budget)
{
    if (text.size() > stringLimit || total > xmlCharacters - text.size())
        return budget.fail("translation-xml-text-limit");
    total += text.size();
    return budget.step();
}

bool readDictionary(const QByteArray& bytes, QHash<QString, QString>& dictionary, Budget& budget)
{
    QXmlStreamReader xml(bytes);
    QStringList elements;
    QString contextName, translated, capture;
    QString captureElement;
    int captureDepth = -1;
    int contexts = 0, tokens = 0, messages = 0;
    qint64 characters = 0;
    bool rootSeen = false, rootClosed = false, nameSeen = false, translationSeen = false;
    bool skipTranslation = false, doctypeSeen = false;
    while (!xml.atEnd()) {
        if (!budget.step() || ++tokens > 524288) return budget.fail("translation-xml-token-limit");
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (!budget.step()) return false;
        if (token == QXmlStreamReader::DTD) {
            const QString declaration = xml.text().toString();
            QString normalized = declaration.simplified();
            normalized.replace(QStringLiteral("TS >"), QStringLiteral("TS>"));
            if (!addXmlText(declaration, characters, budget)) return false;
            if (doctypeSeen || rootSeen || normalized != QStringLiteral("<!DOCTYPE TS>")
                || xml.dtdName() != QStringLiteral("TS") || !xml.dtdPublicId().isEmpty()
                || !xml.dtdSystemId().isEmpty() || !xml.entityDeclarations().isEmpty()
                || !xml.notationDeclarations().isEmpty())
                return budget.fail("translation-xml-dtd-forbidden");
            doctypeSeen = true;
        } else if (token == QXmlStreamReader::EntityReference) {
            return budget.fail("translation-xml-entity-forbidden");
        } else if (token == QXmlStreamReader::StartElement) {
            if (captureDepth >= 0) return budget.fail("translation-xml-scalar-nested");
            const QString name = xml.name().toString();
            if (!addXmlText(name, characters, budget) || elements.size() >= 64
                || !xml.namespaceUri().isEmpty() || !xml.prefix().isEmpty()
                || !xml.namespaceDeclarations().isEmpty())
                return budget.fail("translation-xml-structure-invalid");
            for (const auto& attribute : xml.attributes()) {
                if (!budget.step() || attribute.name().size() > keyLimit
                    || !addXmlText(attribute.name().toString(), characters, budget)
                    || !addXmlText(attribute.value().toString(), characters, budget)
                    || !attribute.namespaceUri().isEmpty()) return budget.fail("translation-xml-attribute-invalid");
            }
            const QString parent = elements.isEmpty() ? QString() : elements.last();
            if (elements.isEmpty()) {
                if (rootSeen || rootClosed || name != QStringLiteral("TS")) return budget.fail("translation-xml-root-invalid");
                rootSeen = true;
            } else if (name == QStringLiteral("context")) {
                if (parent != QStringLiteral("TS") || elements.size() != 1 || ++contexts > dictionaryLimit)
                    return budget.fail("translation-xml-context-invalid");
                contextName.clear(); translated.clear(); nameSeen = false; translationSeen = false;
                skipTranslation = false; messages = 0;
            } else if (name == QStringLiteral("message")) {
                if (parent != QStringLiteral("context") || elements.size() != 2 || ++messages != 1
                    || (xml.attributes().hasAttribute(QStringLiteral("numerus"))
                        && xml.attributes().value(QStringLiteral("numerus")) != QStringLiteral("no")))
                    return budget.fail("translation-xml-message-invalid");
            } else if (name == QStringLiteral("name") && parent == QStringLiteral("context")) {
                if (nameSeen || elements.size() != 2) return budget.fail("translation-xml-name-invalid");
                nameSeen = true; captureElement = name; capture.clear(); captureDepth = elements.size() + 1;
            } else if ((name == QStringLiteral("source") || name == QStringLiteral("translation"))
                       && parent == QStringLiteral("message")) {
                if (elements.size() != 3) return budget.fail("translation-xml-message-invalid");
                if (name == QStringLiteral("translation")) {
                    if (translationSeen) return budget.fail("translation-xml-translation-duplicate");
                    translationSeen = true;
                    const QString type = xml.attributes().value(QStringLiteral("type")).toString();
                    if (!type.isEmpty() && type != QStringLiteral("unfinished") && type != QStringLiteral("obsolete")
                        && type != QStringLiteral("vanished")) return budget.fail("translation-xml-translation-type-invalid");
                    skipTranslation = !type.isEmpty();
                }
                captureElement = name; capture.clear(); captureDepth = elements.size() + 1;
            } else {
                // Only ordinary, non-semantic TS annotations are ignored. They
                // remain in all token/depth/text budgets, including unknown text.
                static const QSet<QString> annotations = {QStringLiteral("comment"), QStringLiteral("extracomment"),
                    QStringLiteral("translatorcomment"), QStringLiteral("location"), QStringLiteral("oldsource"),
                    QStringLiteral("oldcomment"), QStringLiteral("userdata"), QStringLiteral("dependencies"),
                    QStringLiteral("dependency"), QStringLiteral("defaultcodec")};
                if (!annotations.contains(name)) return budget.fail("translation-xml-element-unsupported");
            }
            elements.append(name);
        } else if (token == QXmlStreamReader::ProcessingInstruction) {
            if (!addXmlText(xml.processingInstructionTarget().toString(), characters, budget)
                || !addXmlText(xml.processingInstructionData().toString(), characters, budget)) return false;
        } else if (token == QXmlStreamReader::Characters || token == QXmlStreamReader::Comment) {
            const QString text = xml.text().toString();
            if (!addXmlText(text, characters, budget)) return false;
            if (token == QXmlStreamReader::Characters && captureDepth >= 0) {
                if (capture.size() > stringLimit - text.size()) return budget.fail("translation-xml-text-limit");
                capture += text;
            } else if (token == QXmlStreamReader::Characters && elements.isEmpty() && !text.trimmed().isEmpty()) {
                return budget.fail("translation-xml-outside-root");
            }
        } else if (token == QXmlStreamReader::EndElement) {
            if (elements.isEmpty() || elements.last() != xml.name().toString()) return budget.fail("translation-xml-structure-invalid");
            if (captureDepth == elements.size()) {
                if (captureElement == QStringLiteral("name")) contextName = capture;
                else if (captureElement == QStringLiteral("translation")) translated = capture;
                captureDepth = -1; capture.clear(); captureElement.clear();
            }
            if (elements.last() == QStringLiteral("context")) {
                if (!nameSeen || contextName.isEmpty() || contextName.size() > pathLimit || messages != 1 || !translationSeen)
                    return budget.fail("translation-xml-context-incomplete");
                if (!skipTranslation && !translated.isEmpty()) {
                    if (dictionary.contains(contextName) || dictionary.size() >= dictionaryLimit)
                        return budget.fail("translation-xml-context-duplicate");
                    dictionary.insert(contextName, translated);
                }
            }
            if (elements.last() == QStringLiteral("TS")) rootClosed = true;
            elements.removeLast();
        }
    }
    if (!budget.step() || xml.hasError() || !rootSeen || !rootClosed || !elements.isEmpty() || dictionary.isEmpty())
        return budget.fail("translation-xml-incomplete");
    return true;
}

struct Rule {
    int list = -1;
    int reference = -1;
    QString key;
    QStringList fields, globals;
    QVector<QPair<QString, int>> items;
    bool hasItems = false;
};

bool pointerName(const QString& reference, QString& name, Budget& budget)
{
    if (!reference.startsWith(QStringLiteral("#/$defs/"))) return budget.fail("translation-reference-invalid");
    const QString token = reference.mid(8);
    if (token.isEmpty() || token.contains(QLatin1Char('/'))) return budget.fail("translation-reference-invalid");
    name.clear();
    for (int i = 0; i < token.size(); ++i) {
        if (!budget.step()) return false;
        if (token.at(i) != QLatin1Char('~')) name += token.at(i);
        else {
            if (++i >= token.size() || (token.at(i) != QLatin1Char('0') && token.at(i) != QLatin1Char('1')))
                return budget.fail("translation-reference-invalid");
            name += token.at(i) == QLatin1Char('0') ? QLatin1Char('~') : QLatin1Char('/');
        }
    }
    return true;
}

bool fieldList(const QJsonValue& value, QStringList& fields, Budget& budget)
{
    if (!value.isArray()) return budget.fail("translation-field-list-invalid");
    QSet<QString> seen;
    for (const QJsonValue& entry : value.toArray()) {
        if (!budget.step() || !entry.isString() || entry.toString().isEmpty() || entry.toString().size() > keyLimit
            || seen.contains(entry.toString())) return budget.fail("translation-field-list-invalid");
        seen.insert(entry.toString()); fields.append(entry.toString());
    }
    return true;
}

bool compileRules(const QJsonObject& schema, QVector<Rule>& rules, Budget& budget)
{
    struct Pending { QJsonObject object; bool listBody; bool root; };
    QVector<Pending> pending;
    QHash<QString, int> definitions;
    QJsonObject top = schema;
    if (top.contains(QStringLiteral("$defs")) && !top.value(QStringLiteral("$defs")).isObject())
        return budget.fail("translation-definitions-invalid");
    const QJsonObject defs = top.take(QStringLiteral("$defs")).toObject();
    if (!top.contains(QStringLiteral("items")) && !top.contains(QStringLiteral("$ref")))
        return budget.fail("translation-rules-missing");
    pending.append({top, false, true}); rules.append(Rule());
    for (auto it = defs.constBegin(); it != defs.constEnd(); ++it) {
        if (!budget.step() || it.key().isEmpty() || !it.value().isObject()) return budget.fail("translation-definition-invalid");
        definitions.insert(it.key(), rules.size());
        pending.append({it.value().toObject(), false, false}); rules.append(Rule());
    }
    for (int index = 0; index < pending.size(); ++index) {
        if (!budget.step()) return false;
        const Pending next = pending.at(index);
        Rule rule;
        for (auto it = next.object.constBegin(); it != next.object.constEnd(); ++it) {
            if (!budget.step()) return false;
            const QString key = it.key();
            const QJsonValue value = it.value();
            if (key == QStringLiteral("$ref")) {
                QString name;
                if (!value.isString() || !pointerName(value.toString(), name, budget) || !definitions.contains(name))
                    return budget.fail("translation-reference-missing");
                rule.reference = definitions.value(name);
            } else if (key == QStringLiteral("items")) {
                if (!value.isObject()) return budget.fail("translation-items-invalid");
                rule.hasItems = true;
                const QJsonObject items = value.toObject();
                for (auto item = items.constBegin(); item != items.constEnd(); ++item) {
                    if (!budget.step() || item.key().isEmpty() || !item.value().isObject() || rules.size() >= nodeLimit)
                        return budget.fail("translation-selector-invalid");
                    rule.items.append(qMakePair(item.key(), rules.size()));
                    pending.append({item.value().toObject(), false, false}); rules.append(Rule());
                }
            } else if (key == QStringLiteral("list") && !next.root) {
                if (!value.isObject() || rules.size() >= nodeLimit) return budget.fail("translation-list-invalid");
                rule.list = rules.size();
                pending.append({value.toObject(), true, false}); rules.append(Rule());
            } else if (key == QStringLiteral("key") && next.listBody) {
                if (!value.isString() || value.toString().isEmpty() || value.toString().size() > keyLimit)
                    return budget.fail("translation-list-key-invalid");
                rule.key = value.toString();
            } else if (key == QStringLiteral("translate") && !next.root) {
                if (!fieldList(value, rule.fields, budget)) return false;
            } else if (key == QStringLiteral("translate-global") && !next.root) {
                if (!fieldList(value, rule.globals, budget)) return false;
            } else return budget.fail("translation-rule-operation-unsupported");
        }
        rules[index] = rule;
    }
    return true;
}

bool contextPath(const QString& prefix, const QString& suffix, QString& result, Budget& budget)
{
    if (!budget.step() || prefix.size() >= pathLimit || suffix.size() > pathLimit - prefix.size() - 1)
        return budget.fail("translation-context-path-limit");
    result = prefix + QLatin1Char('/') + suffix;
    return true;
}

struct Interpreter {
    QVector<Node>& nodes;
    const QVector<Rule>& rules;
    const QHash<QString, QString>& dictionary;
    Budget& budget;
    QSet<quint64> active;
    qint64 replacements = 0;

    bool replace(int nodeId, const QString& context) {
        if (!budget.step()) return false;
        const auto found = dictionary.constFind(context);
        if (found == dictionary.constEnd()) return true;
        if (nodes[nodeId].type != QJsonValue::String) return budget.fail("translation-target-not-string");
        if (replacements > replacementCharacters - found.value().size()) return budget.fail("translation-replacement-limit");
        replacements += found.value().size();
        nodes[nodeId].scalar = found.value();
        return true;
    }

    bool fields(int nodeId, const QString& prefix, const QStringList& names, bool global) {
        if (names.isEmpty()) return true;
        if (nodes[nodeId].type != QJsonValue::Object) return budget.fail("translation-target-not-object");
        for (const QString& name : names) {
            if (!budget.step()) return false;
            const auto member = nodes[nodeId].object.constFind(name);
            if (member == nodes[nodeId].object.constEnd()) continue;
            const int valueId = member.value();
            QString base;
            if (!contextPath(global ? QStringLiteral("$globals") : prefix, name, base, budget)) return false;
            if (nodes[valueId].type == QJsonValue::String) {
                QString context = base;
                if (global && !contextPath(base, nodes[valueId].scalar.toString(), context, budget)) return false;
                if (!replace(valueId, context)) return false;
            } else if (nodes[valueId].type == QJsonValue::Array) {
                for (int i = 0; i < nodes[valueId].array.size(); ++i) {
                    if (!budget.step()) return false;
                    const int entry = nodes[valueId].array.at(i);
                    if (nodes[entry].type != QJsonValue::String) return budget.fail("translation-string-array-invalid");
                    QString context;
                    if (!contextPath(base, global ? nodes[entry].scalar.toString() : QString::number(i), context, budget)
                        || !replace(entry, context)) return false;
                }
            } else return budget.fail("translation-field-type-invalid");
        }
        return true;
    }

    bool keyText(int entryId, const QString& key, int index, QString& text) {
        if (!budget.step()) return false;
        if (key.isEmpty() || nodes[entryId].type != QJsonValue::Object || !nodes[entryId].object.contains(key)) {
            text = QString::number(index); return true;
        }
        const Node& value = nodes.at(nodes[entryId].object.value(key));
        if (value.type == QJsonValue::String) { text = value.scalar.toString(); return true; }
        if (value.type != QJsonValue::Double || !std::isfinite(value.scalar.toDouble()))
            return budget.fail("translation-list-key-type-invalid");
        // QJson stores numbers as doubles, not their original lexical form.
        // Use Qt's locale-independent JSON decimal representation, including
        // integral numeric indices; never QString::toString()'s empty result.
        if (!budget.step()) return false;
        QJsonArray number; number.append(value.scalar);
        const QByteArray json = QJsonDocument(number).toJson(QJsonDocument::Compact);
        if (!budget.step() || json.size() < 3) return budget.fail("translation-list-key-number-invalid");
        text = QString::fromLatin1(json.constData() + 1, json.size() - 2);
        return true;
    }

    bool apply(int ruleId, int nodeId, const QString& prefix, int depth = 0) {
        if (!budget.step() || depth >= 256) return budget.fail("translation-rule-depth-limit");
        const quint64 identity = (quint64(quint32(ruleId)) << 32) | quint32(nodeId);
        if (active.contains(identity)) return budget.fail("translation-reference-no-data-progress");
        active.insert(identity);
        // The active pair belongs to this invocation only. Removal also occurs
        // on every failure, without relying on a translation prefix's uniqueness.
        struct Exit { QSet<quint64>& set; quint64 key; ~Exit() { set.remove(key); } } exit{active, identity};
        const Rule& rule = rules.at(ruleId);
        if (rule.list >= 0) {
            if (nodes[nodeId].type != QJsonValue::Array) return budget.fail("translation-list-target-invalid");
            const Rule& child = rules.at(rule.list);
            for (int i = 0; i < nodes[nodeId].array.size(); ++i) {
                if (!budget.step()) return false;
                const int entry = nodes[nodeId].array.at(i);
                QString key, next;
                if (!keyText(entry, child.key, i, key) || !contextPath(prefix, key, next, budget)
                    || !apply(rule.list, entry, next, depth + 1)) return false;
            }
        }
        if (!fields(nodeId, prefix, rule.fields, false) || !fields(nodeId, prefix, rule.globals, true)) return false;
        if (rule.hasItems) {
            if (nodes[nodeId].type != QJsonValue::Object) return budget.fail("translation-items-target-invalid");
            for (const auto& selector : rule.items) {
                if (!budget.step()) return false;
                if (selector.first == QStringLiteral("*")) {
                    for (auto it = nodes[nodeId].object.constBegin(); it != nodes[nodeId].object.constEnd(); ++it) {
                        QString next;
                        if (!contextPath(prefix, it.key(), next, budget) || !apply(selector.second, it.value(), next, depth + 1)) return false;
                    }
                } else if (nodes[nodeId].object.contains(selector.first)) {
                    QString next;
                    if (!contextPath(prefix, selector.first, next, budget)
                        || !apply(selector.second, nodes[nodeId].object.value(selector.first), next, depth + 1)) return false;
                }
            }
        }
        return rule.reference < 0 || apply(rule.reference, nodeId, prefix, depth + 1);
    }
};

bool rebuild(const QVector<Node>& nodes, QJsonDocument& document, Budget& budget)
{
    QVector<QJsonValue> values(nodes.size());
    qint64 characters = 0;
    for (int i = nodes.size() - 1; i >= 0; --i) {
        if (!budget.step()) return false;
        const Node& node = nodes.at(i);
        if (node.type == QJsonValue::Object) {
            QJsonObject object;
            for (auto it = node.object.constBegin(); it != node.object.constEnd(); ++it) {
                if (!budget.step()) return false;
                if (characters > jsonCharacters - it.key().size()) return budget.fail("translation-output-text-limit");
                characters += it.key().size();
                object.insert(it.key(), values.at(it.value()));
                // Every physical node has one parent. The parent now owns its
                // inserted value; do not retain another full subtree per level.
                values[it.value()] = QJsonValue();
                if (!budget.step()) return false;
            }
            values[i] = object;
        } else if (node.type == QJsonValue::Array) {
            QJsonArray array;
            for (int child : node.array) {
                if (!budget.step()) return false;
                array.append(values.at(child));
                values[child] = QJsonValue();
                if (!budget.step()) return false;
            }
            values[i] = array;
        } else {
            if (node.type == QJsonValue::String) {
                const int size = node.scalar.toString().size();
                if (size > stringLimit || characters > jsonCharacters - size)
                    return budget.fail("translation-output-text-limit");
                characters += size;
            }
            values[i] = node.scalar;
        }
    }
    document.setObject(values.at(0).toObject());
    return budget.step();
}

bool writeOutput(const QByteArray& bytes, QString& path, Budget& budget)
{
    if (!budget.step() || bytes.size() > byteLimit) return budget.fail("translation-output-byte-limit");
    QTemporaryFile output(QDir::temp().filePath(QStringLiteral("qgc-component-translation-XXXXXX.json")));
    if (!output.open() || !budget.step()) return budget.fail("translation-output-open-failed");
    for (qint64 offset = 0; offset < bytes.size();) {
        if (!budget.step()) return false;
        const qint64 count = qMin<qint64>(chunkSize, bytes.size() - offset);
        if (output.write(bytes.constData() + offset, count) != count || !budget.step())
            return budget.fail("translation-output-write-failed");
        offset += count;
    }
    if (!budget.step() || !output.flush() || !budget.step() || output.error() != QFileDevice::NoError
        || output.size() != bytes.size()) return budget.fail("translation-output-flush-failed");
    const QString completed = output.fileName();
    output.close();
    if (!budget.step() || output.isOpen() || output.error() != QFileDevice::NoError)
        return budget.fail("translation-output-close-failed");
    output.setAutoRemove(false);
    path = completed;
    return true;
}
}

bool ComponentTranslationEngine::resolveSummary(const QString& path, const QString& locale,
    const std::atomic_bool& cancelled, const QElapsedTimer& clock, QString& url, QString& error)
{
    const QString inputPath = path;
    const QString inputLocale = locale;
    url.clear(); error.clear();
    Budget budget{cancelled, clock, error};
    if (!budget.step() || inputLocale.isEmpty() || inputLocale.size() > keyLimit || inputLocale.contains(QChar(0)))
        return budget.fail("translation-locale-invalid");
    QByteArray bytes;
    QJsonDocument document;
    QVector<Node> nodes;
    if (!readFile(inputPath, bytes, budget) || !parseJson(bytes, document, nodes, budget)) return false;
    const QJsonValue entry = document.object().value(inputLocale);
    if (!entry.isObject() || !entry.toObject().value(QStringLiteral("url")).isString())
        return budget.fail("translation-locale-not-found");
    const QString candidate = entry.toObject().value(QStringLiteral("url")).toString();
    if (!budget.step() || candidate.isEmpty() || candidate.size() > 4096
        || (!candidate.startsWith(QStringLiteral("http://")) && !candidate.startsWith(QStringLiteral("https://"))))
        return budget.fail("translation-summary-url-invalid");
    for (const QChar ch : candidate) {
        if (!budget.step() || ch.unicode() <= 0x20 || ch.unicode() == 0x7f)
            return budget.fail("translation-summary-url-invalid");
    }
    const QUrl parsed(candidate, QUrl::StrictMode);
    if (!parsed.isValid() || parsed.isRelative() || parsed.host().isEmpty() || !parsed.userInfo().isEmpty()
        || (parsed.scheme() != QStringLiteral("http") && parsed.scheme() != QStringLiteral("https")))
        return budget.fail("translation-summary-url-invalid");
    if (!budget.step()) return false;
    url = candidate;
    return true;
}

bool ComponentTranslationEngine::translate(const QString& jsonPath, const QString& tsPath,
    const std::atomic_bool& cancelled, const QElapsedTimer& clock, QString& outputPath, QString& error)
{
    const QString inputJsonPath = jsonPath;
    const QString inputTsPath = tsPath;
    outputPath.clear(); error.clear();
    Budget budget{cancelled, clock, error};
    QByteArray json, xml;
    QJsonDocument document;
    QVector<Node> nodes;
    if (!readFile(inputJsonPath, json, budget) || !parseJson(json, document, nodes, budget)
        || !readFile(inputTsPath, xml, budget)) return false;
    const QJsonValue translation = document.object().value(QStringLiteral("translation"));
    if (!translation.isObject() || translation.toObject().isEmpty()) return budget.fail("translation-rules-missing");
    QHash<QString, QString> dictionary;
    if (!readDictionary(xml, dictionary, budget)) return false;
    QVector<Rule> rules;
    if (!compileRules(translation.toObject(), rules, budget)) return false;
    Interpreter interpreter{nodes, rules, dictionary, budget, {}, 0};
    if (!interpreter.apply(0, 0, QString()) || !rebuild(nodes, document, budget) || !budget.step()) return false;
    const QByteArray output = document.toJson(QJsonDocument::Indented);
    if (!budget.step()) return false;
    return writeOutput(output, outputPath, budget);
}
