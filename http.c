/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/intuition.h>
#include <classes/multimedia/video.h>
#include <classes/multimedia/streams.h>

#include "http.h"
#include "globaldefines.h"

VOID HttpGetRequest(VOID)
{
	struct Library *IntuitionBase;

	if((IntuitionBase = OpenLibrary("intuition.library", 0)))
	{
		struct Library *MultimediaBase;

		if((MultimediaBase = OpenLibrary("multimedia/multimedia.class", 53)))
		{
			struct Library *HttpStreamBase;

			if((HttpStreamBase = OpenLibrary("multimedia/http.stream", 51)))
			{
				struct HttpGet *startup_msg;

				if(NewGetTaskAttrs(NULL, &startup_msg, sizeof(struct HttpGet *), TASKINFOTYPE_STARTUPMSG, TAG_DONE) && startup_msg)
				{
					Object *http = NULL;

					http = NewObject(NULL, "http.stream",
						MMA_StreamName, (ULONG)startup_msg->hg_Url,
						MMA_Http_UserAgentOverride, (ULONG)startup_msg->hg_UserAgent,
						MMA_Http_RequestType, MMV_Http_RequestType_Get,
					TAG_END);

					if(http)
					{
						QUAD buffer_size = MediaGetPort64(http, 0, MMA_StreamLength);

						if(buffer_size == 0)
							buffer_size = 1024*1024;

						if((buffer_size < 2147483648LL) && (startup_msg->hg_Data = (UBYTE*)AllocVec((LONG)buffer_size, MEMF_PUBLIC | MEMF_CLEAR)))
						{
							startup_msg->hg_DataLen = DoMethod(http, MMM_Pull, 0, (ULONG)startup_msg->hg_Data, (LONG)buffer_size);

							if(startup_msg->hg_DataLen <= 0)
							{
								FreeVec(startup_msg->hg_Data);
								startup_msg->hg_Data = NULL; /* no longer vaild */
							}
						}
						DisposeObject(http);
					}
				}
				CloseLibrary(HttpStreamBase);
			}
			CloseLibrary(MultimediaBase);
		}
		CloseLibrary(IntuitionBase);
	}
}

VOID HttpPostRequest(VOID)
{
	struct Library *IntuitionBase;

	if((IntuitionBase = OpenLibrary("intuition.library", 0)))
	{
		struct Library *MultimediaBase;

		if((MultimediaBase = OpenLibrary("multimedia/multimedia.class", 53)))
		{
			struct Library *HttpStreamBase;

			if((HttpStreamBase = OpenLibrary("multimedia/http.stream", 51)))
			{
				struct HttpPost *startup_msg;

				if(NewGetTaskAttrs(NULL, &startup_msg, sizeof(struct HttpGet *), TASKINFOTYPE_STARTUPMSG, TAG_DONE) && startup_msg)
				{
					Object *http = NULL;

					http = NewObject(NULL, "http.stream",
						MMA_StreamName, (ULONG)startup_msg->hp_Url,
						MMA_Http_UserAgentOverride, (ULONG)startup_msg->hp_UserAgent,
						MMA_Http_RequestType, MMV_Http_RequestType_PostUrl,
						MMA_Http_PostData, (ULONG)startup_msg->hp_Post,
					TAG_END);

					if(http)
					{
						QUAD buffer_size = MediaGetPort64(http, 0, MMA_StreamLength);

						if(buffer_size == 0)
							buffer_size = 1024*1024;

						if((buffer_size < 2147483648LL) && (startup_msg->hp_Data = (UBYTE*)AllocVec((LONG)buffer_size, MEMF_PUBLIC | MEMF_CLEAR)))
						{
							startup_msg->hp_DataLen = DoMethod(http, MMM_Pull, 0, (ULONG)startup_msg->hp_Data, (LONG)buffer_size);

							if(startup_msg->hp_DataLen <= 0)
							{
								FreeVec(startup_msg->hp_Data);
								startup_msg->hp_Data = NULL; /* no longer vaild */
							}
						}
						DisposeObject(http);
					}
				}
				CloseLibrary(HttpStreamBase);
			}
			CloseLibrary(MultimediaBase);
		}
		CloseLibrary(IntuitionBase);
	}
}
