// test_verdict.cpp
// TC22 - TC24: Module 8 (verdict check) and the dataset files.

#include "rtts/dataset.hpp"
#include "rtts/experiments.hpp"
#include "rtts/generator.hpp"
#include "rtts/verdict.hpp"
#include "test_framework.hpp"

using namespace rtts;

TEST(TC22_verdict_on_sample_set) {
    Verdict v = checkTaskSet(sampleTaskSet());
    CHECK(v.agrees());
    CHECK(v.rmSim && v.edfSim && v.sameSchedule);
}

TEST(TC23_verdict_experiment_has_no_mismatches) {
    auto records = makeDataset(10);   // 3 * 11 * 10 = 330 sets
    VerdictReport report = runVerdictExperiment(records);
    CHECK_EQ(report.totalSets, 330);
    CHECK_EQ(report.rows.size(), 33u);
    CHECK(report.mismatches.empty());

    // Sanity: with 5 tasks at U = 0.5 everything is schedulable by EDF. (With 20
    // tasks, rounding C up to at least 1 tick can push the real U above 1.)
    for (const VerdictRow& row : report.rows)
        if (row.n == 5 && row.targetU < 0.51) CHECK(row.edfSim > 0.99);
}

TEST(TC24_dataset_csv_round_trip) {
    auto records = makeDataset(2);
    saveDatasetCsv(records, "build_test_tmp/tasksets.csv");
    auto loaded = loadDatasetCsv("build_test_tmp/tasksets.csv");
    CHECK_EQ(loaded.size(), records.size());
    CHECK_EQ(loaded[5].tasks.size(), records[5].tasks.size());
    CHECK_EQ(loaded[5].tasks[0].wcet, records[5].tasks[0].wcet);
}
