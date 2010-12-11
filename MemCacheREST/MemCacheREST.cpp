#define _WINSOCKAPI_
#include <windows.h>
#include <sal.h>
#include <httpserv.h>
#include <winsock2.h>
#include <stdio.h>
#include <memcached.h>
#include <string.h>

// TODO: Read this out of registry
#define SERVER_NAME "127.0.0.1"
#define SERVER_PORT 11211

// Create the basic module, to be used by the factory
class MemCacheREST : public CHttpModule {
    public:
		// Handles request start at the very start of the request pipeline
        REQUEST_NOTIFICATION_STATUS OnBeginRequest(IN IHttpContext * httpContext, IN IHttpEventProvider * eventProvider) {
            UNREFERENCED_PARAMETER(eventProvider);

            IHttpRequest* httpRequest = httpContext->GetRequest(); // Retrieve a pointer to the Request.
			IHttpResponse* httpResponse = httpContext->GetResponse(); // Retrieve a pointer to the Response.
    
	        HRESULT hr;

			if (httpResponse != NULL) {
				PCWSTR forwardedUrl = httpRequest->GetForwardedUrl();
				if (forwardedUrl != NULL) {
					httpResponse->Clear();
					// Set the MIME type to plain text.
					httpResponse->SetHeader(HttpHeaderContentType,"text/xml",(USHORT)strlen("text/plain"),TRUE);

					// Create a string with the response.
					char * pszBuffer = "";
					strcat(pszBuffer, "<item>foo</item>");
					// Create a data chunk.
					HTTP_DATA_CHUNK dataChunk;
					// Set the chunk to a chunk in memory.
					dataChunk.DataChunkType = HttpDataChunkFromMemory;
					// Buffer for bytes written of data chunk.
					DWORD cbSent;

					// Set the chunk to the buffer.
					dataChunk.FromMemory.pBuffer = (PVOID) pszBuffer;
					// Set the chunk size to the buffer size.
					dataChunk.FromMemory.BufferLength = (USHORT) strlen(pszBuffer);
					// Insert the data chunk into the response.
					hr = httpResponse->WriteEntityChunks(&dataChunk,1,FALSE,TRUE,&cbSent);

					// Test for an error.
					if (FAILED(hr)) {
						// Set the HTTP status.
						httpResponse->SetStatus(500,"Server Error",0,hr);
					}

					// End additional processing.
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}
			}


			/*
            if (httpRequest != NULL) { // Did we get a working request object? 
                PCWSTR forwardedUrl = httpRequest->GetForwardedUrl();

                if (forwardedUrl != 0) { // We got a valid URL
                    memcached_return rc;
                    memcached_st* memc;
                    char* key = "Foo";
					char* value = "Bar";
					char* return_value;
					char* site;
					char* session;
                    size_t return_value_length = 0;
                    uint32_t flags = 0;

                    memc = memcached_create(NULL);
                    rc = memcached_server_add(memc, SERVER_NAME, SERVER_PORT);
                    //printf("server add: %s\n", memcached_strerror(memc, rc));

                    rc = memcached_set(memc, key, strlen(key), value, strlen(value), 0, 0);
                    //printf("set '%s' to '%s': %s\n", KEY, VALUE1, memcached_strerror(memc, rc));

                    return_value = memcached_get(memc, key, strlen(key), &return_value_length, &flags, &rc);
					return RQ_NOTIFICATION_FINISH_REQUEST;
                }
            }
			*/
			

            // Return processing to the pipeline.
            return RQ_NOTIFICATION_CONTINUE;
        }
};



// Create the module's class factory, this is because it is a per-request module, and not global
class MemCacheRESTFactory : public IHttpModuleFactory {
    public:
        HRESULT GetHttpModule(OUT CHttpModule ** pModule, IN IModuleAllocator * moduleAllocator) {
            UNREFERENCED_PARAMETER(moduleAllocator);

            MemCacheREST * mcr = new MemCacheREST; // Create a new instance of the MemCacheREST object. 
            if (!mcr) { // Return an error if the factory cannot create the instance.
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            } else {
                *pModule = mcr; // Return a pointer to the module. (ie: pass it out via **pModule) 
                mcr = NULL; // cleanup

                return S_OK; // Return a success status.
            }
        }

        void Terminate() {
            delete this; // IMPORTANT: Remove the class from memory.
        }
};

// Create the module's exported registration function.
HRESULT __stdcall RegisterModule(DWORD dwServerVersion, IHttpModuleRegistrationInfo  * mcrInfo, IHttpServer * pGlobalInfo) {
    UNREFERENCED_PARAMETER(dwServerVersion);
    UNREFERENCED_PARAMETER(pGlobalInfo);

    return mcrInfo->SetRequestNotifications(new MemCacheRESTFactory, RQ_BEGIN_REQUEST, 0); // Set the request notifications and exit.
}