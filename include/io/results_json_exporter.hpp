#ifndef RESULTS_JSON_EXPORTER_HPP
#define RESULTS_JSON_EXPORTER_HPP

#include <string>

#include "external/rapidjson/document.h"

#include "analysis-setup/residue_information_provider.hpp"
#include "statistics/summary_statistics.hpp"


/*!
 *
 */
class ResultsJsonExporter
{
    public:
       
        // constructor:
        ResultsJsonExporter();

        // interface for adding to output:
        void addPathwaySummary(
                std::string name,
                const SummaryStatistics &summary);
        void addSupportPoints(
                const std::vector<real> &supportPoints);
        void addPathwayProfile(
                std::string name,
                const std::vector<SummaryStatistics> &profile);
        void addTimeStamps(
                const std::vector<real> &timeStamps);
        void addPathwayScalarTimeSeries(
                std::string name,
                const std::vector<real> &timeSeries);
        void addPathwayGridPoints(
                const std::vector<real> &timeStamps,
                const std::vector<real> &supportPoints);
        void addPathwayProfileTimeSeries(
                std::string name,
                const std::vector<std::vector<real>> &timeSeries);
        void addResidueInformation(
                const std::vector<int> &resId,
                const ResidueInformationProvider &resInf);
        void addResidueSummary(
                std::string name,
                const std::vector<SummaryStatistics> &resSummary);

        // interface for writing to file:
        void write(std::string filename);

    private:

        // helper function to convert a string to a rapidjson value:
        inline rapidjson::Value toVal(const std::string &str);

        // function for populating the reproducibility info with values:
        rapidjson::Value reproducibilityInformation();

        // overall output document:
        rapidjson::Document doc_;
};

#endif
