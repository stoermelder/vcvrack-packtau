#include "rack.hpp"
#include "pluginsettings.hpp"


StoermelderSettings pluginSettings;

StoermelderSettings::~StoermelderSettings() {
    if (mbV06favouritesJ) json_decref(mbV06favouritesJ);
}

void StoermelderSettings::saveToJson() {
    json_t* settingsJ = json_object();
    json_object_set(settingsJ, "mbV06favourites", mbV06favouritesJ);
    json_object_set(settingsJ, "mbV1zoom", json_real(mbV1zoom));

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

    mbV06favouritesJ = json_copy(json_object_get(settingsJ, "mbV06favourites"));
    json_t* mbV1zoomJ = json_object_get(settingsJ, "mbV1zoom");
    mbV1zoom = json_real_value(mbV1zoomJ);

    fclose(file);
    json_decref(settingsJ);
}