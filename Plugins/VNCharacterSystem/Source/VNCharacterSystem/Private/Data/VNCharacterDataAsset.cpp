#include "Data/VNCharacterDataAsset.h"
#include "VNCharacterSystemModule.h"

UVNCharacterDataAsset::UVNCharacterDataAsset()
{
	VN_LOG_DEBUG(TEXT("VNCharacterDataAsset constructor called"));
	
	// Инициализация базовых значений для всех конфигураций
	
	// Skeletal Mesh конфигурации уже инициализированы в структурах
	// Sprite конфигурации уже инициализированы в структурах
	
	// Можно добавить дополнительную инициализацию если нужно
}