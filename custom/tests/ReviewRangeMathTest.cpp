#include <QtTest>

#include "../src/Review/ReviewRangeMath.h"

class ReviewRangeMathTest : public QObject
{
    Q_OBJECT

private slots:
    void protectsConfiguredReserveAndTaskBudget()
    {
        ReviewRangeMath::Inputs input;
        input.batteryPercent = 80;
        input.reservePercent = 20;
        input.remainingSeconds = 2400;
        input.workSeconds = 300;
        input.workPowerFactor = 1.3;
        input.cruisePowerFactor = 1.5;
        input.cruiseSpeed = 10;
        input.windSpeed = 2;

        QCOMPARE(ReviewRangeMath::usableSeconds(input), 1410.0);
        QCOMPARE(ReviewRangeMath::radiusMeters(input), 4512.0);
        QCOMPARE(ReviewRangeMath::returnMarginSeconds(input, 2000), 1035.0);

        input.reservePercent = 30;
        QVERIFY(ReviewRangeMath::radiusMeters(input) < 4512.0);
    }

    void rejectsUnavailableOrUnsafeInputs()
    {
        ReviewRangeMath::Inputs input;
        input.batteryPercent = 40;
        input.reservePercent = 20;
        input.remainingSeconds = 1000;
        input.workSeconds = 300;
        input.workPowerFactor = 1.3;
        input.cruisePowerFactor = 1.5;
        input.cruiseSpeed = 10;
        input.windSpeed = qQNaN();

        QVERIFY(qIsNaN(ReviewRangeMath::radiusMeters(input)));
        input.windSpeed = 10;
        QVERIFY(qIsNaN(ReviewRangeMath::radiusMeters(input)));
        input.windSpeed = 2;
        input.batteryPercent = 15;
        QVERIFY(qIsNaN(ReviewRangeMath::radiusMeters(input)));
    }
};

QTEST_APPLESS_MAIN(ReviewRangeMathTest)

#include "ReviewRangeMathTest.moc"
