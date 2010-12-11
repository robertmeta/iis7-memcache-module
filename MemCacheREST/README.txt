For this POC -- requires memcache running locally and  the memcached.dll from the libmemcached-source/libmemcached directory must be in a common location (system32 works well). 

Use "c:\windows\system32\inetsrv\appcmd install module /name:MemCacheREST /image:C:\Users\rmelton\Downloads\MemCacheREST\Debug\MemCacheREST.dll /add:true /lock:true" to add the DLL to IIS7