#pragma once


struct StoermelderSettings {
	json_t* mbModelsJ;
	float mbV1zoom = 0.85f;
	int mbV1sort = 0;
	
	~StoermelderSettings();
	void saveToJson();
	void readFromJson();
};