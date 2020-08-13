#pragma once


struct StoermelderSettings {
	json_t* mbModelsJ;
	float mbV1zoom = 0.85f;
	
	~StoermelderSettings();
	void saveToJson();
	void readFromJson();
};