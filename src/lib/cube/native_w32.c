#include "linkedList.h"
#include <Windows.h>
#include "imagehlp.h"
#include "native.h"


linked_list *list_symbols(const char *path)
{
	linked_list *list = linked_list_create();
	bool is_func = true;

	LOADED_IMAGE image;
	if (MapAndLoad(path, NULL, &image, TRUE, FALSE))
	{
		// get the export table
		ULONG size;
		PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(image.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

		PIMAGE_SECTION_HEADER *pHeader = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER));
		memset(pHeader, '\0', sizeof(IMAGE_SECTION_HEADER));

		// get the names address
		PULONG names = (PULONG)ImageRvaToVa(image.FileHeader, image.MappedAddress, exports->AddressOfNames, pHeader);

		for (ULONG i = 0; i < exports->NumberOfNames; i++)
		{
			// get a given name
			PSTR name = (PSTR)ImageRvaToVa(image.FileHeader, image.MappedAddress, names[i], pHeader);

			symbol_t *sym = (symbol_t *)malloc(sizeof(symbol_t));
			sym->is_func = is_func;
			sym->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
			strcpy(sym->name, name);
			linked_list_add(list, sym);
		}

		UnMapAndLoad(&image); // commit & write
	}
    return list;
}