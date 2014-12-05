/* 
   Implementation of ResourceUtils.h
   Contains function for loading a resource by name and converting it to a BBitmap.
   Distributed as part of the resource editor Style from Realize productionS.
   There are absolutely no restrictions on the usage/re-distribution of this file.

   		 Realize productionS
   Superior software for a superior OS
*/

#include <be/app/Application.h>
#include <be/interface/Bitmap.h>
#include <be/storage/File.h>
#include <be/storage/Resources.h>
#include <string.h>
#include <type_traits>

BBitmap *FetchStyleResource(const char *rcName)
{
	int i=0;
	int32 id;
	char *name;
	size_t length;
	type_code type;
	void *data;
	
	// Initialize the file
	app_info info;
	
	if (be_app->GetAppInfo(&info)!=B_OK)
		return NULL;
		
	entry_ref ref=info.ref;
	
	BFile file(&ref, B_READ_ONLY);
	
	if (file.InitCheck()!=B_OK)
		return NULL;
		
	BResources rcFile;
	
	if (rcFile.SetTo(&file)!=B_OK)
		return NULL;
		
	// Search for resource
	while(true)
		if (rcFile.GetResourceInfo(i++, &type, &id, &name, &length))
		{
			if (strcmp(name, rcName)==0)
				{
					data=rcFile.LoadResource(type, name, &length);
					break;
				}			
		}
		else 
			return NULL;

	if (!data)
		return NULL;
		
	// Instantiate BBitmap object from resource data
	BMessage msg;
	
	if(msg.Unflatten((const char*)data)!=B_OK)
		return NULL;
		
	BArchivable *unarchived = instantiate_object(&msg);
	
	if(unarchived)
	{
		BBitmap *bmp = (BBitmap *)unarchived;

		if (bmp)
			return bmp;
		else
			delete unarchived;
	}
	
	return NULL;	
}
