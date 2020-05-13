#pragma once


struct StoermelderSettings {
	json_t* mbV06favouritesJ;
	float mbV1zoom = 0.85f;
	
	~StoermelderSettings();
	void saveToJson();
	void readFromJson();
};