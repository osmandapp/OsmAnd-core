#ifndef _OSMAND_SKIA_WP8_FUNCTIONS_REPLACEMENTS_H_
#define _OSMAND_SKIA_WP8_FUNCTIONS_REPLACEMENTS_H_

#define CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName) \
    CreateEventEx((lpEventAttributes), (lpName), ((bManualReset)?CREATE_EVENT_MANUAL_RESET:0)|((bInitialState)?CREATE_EVENT_INITIAL_SET:0), EVENT_MODIFY_STATE)
	
#define WaitForSingleObject(hHandle, dwMilliseconds) \
    WaitForSingleObjectEx((hHandle), (dwMilliseconds), FALSE)

#define FindFirstFileW(lpFileName, lpFindFileData) \
    FindFirstFileExW((lpFileName), FindExInfoStandard, (lpFindFileData), FindExSearchNameMatch, NULL, 0)

#endif // _OSMAND_SKIA_WP8_FUNCTIONS_REPLACEMENTS_H_
