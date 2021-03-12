#include "rack.hpp"
#include "pluginsettings.hpp"


StoermelderSettings pluginSettings;

StoermelderSettings::~StoermelderSettings() {
    if (mbModelsJ) json_decref(mbModelsJ);
}

void StoermelderSettings::saveToJson() {
    json_t* settingsJ = json_object();
    json_object_set(settingsJ, "mbMode", json_integer(mbMode));
    json_object_set(settingsJ, "mbModels", mbModelsJ);
    json_object_set(settingsJ, "mbV1zoom", json_real(mbV1zoom));
    json_object_set(settingsJ, "mbV1sort", json_integer(mbV1sort));
    json_object_set(settingsJ, "mbV1hideBrands", json_boolean(mbV1hideBrands));
    json_object_set(settingsJ, "mbV1searchDescriptions", json_boolean(mbV1searchDescriptions));

    std::string settingsFilename = rack::asset::user("Stoermelder-PT.json");
    FILE* file = fopen(settingsFilename.c_str(), "w");
    if (file) {
        json_dumpf(settingsJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
        fclose(file);
    }
    json_decref(settingsJ);
}

void StoermelderSettings::readFromJson() {
    std::string settingsFilename = rack::asset::user("Stoermelder-PT.json");
    FILE* file = fopen(settingsFilename.c_str(), "r");
    if (!file) {
        saveToJson();
        return;
    }

    json_error_t error;
    json_t* settingsJ = json_loadf(file, 0, &error);
    if (!settingsJ) {
        // invalid setting json file
        fclose(file);
        saveToJson();
        return;
    }

    json_t* mbModeJ = json_object_get(settingsJ, "mbMode");
    mbMode = json_integer_value(mbModeJ);
    json_t* fmJ = json_object_get(settingsJ, "mbV06favourites");
    if (!fmJ) fmJ = json_object_get(settingsJ, "mbModels");
    mbModelsJ = json_copy(fmJ);
    json_t* mbV1zoomJ = json_object_get(settingsJ, "mbV1zoom");
    mbV1zoom = json_real_value(mbV1zoomJ);
    json_t* mbV1sortJ = json_object_get(settingsJ, "mbV1sort");
    if (mbV1sortJ) mbV1sort = json_integer_value(mbV1sortJ);
    json_t* mbV1hideBrandsJ = json_object_get(settingsJ, "mbV1hideBrands");
    if (mbV1hideBrandsJ) mbV1hideBrands = json_boolean_value(mbV1hideBrandsJ);
    json_t* mbV1searchDescriptionsJ = json_object_get(settingsJ, "mbV1searchDescriptions");
    if (mbV1searchDescriptionsJ) mbV1searchDescriptions = json_boolean_value(mbV1searchDescriptionsJ);

    fclose(file);
    json_decref(settingsJ);
}