#pragma once


struct StoermelderSettings {
	int mbMode = -1;
	json_t* mbModelsJ;
	float mbV1zoom = 0.85f;
	int mbV1sort = 0;
	
	~StoermelderSettings();
	void saveToJson();
	void readFromJson();
};