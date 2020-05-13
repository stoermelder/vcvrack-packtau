#pragma once


struct StoermelderSettings {
	json_t* mbV06favouritesJ;
	float mbV1zoom;

	void saveToJson();
	void readFromJson();
};