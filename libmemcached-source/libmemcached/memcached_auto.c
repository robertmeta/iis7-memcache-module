#include "common.h"

static memcached_return memcached_auto(memcached_st *ptr, 
                                       const char *verb,
                                       const char *key, size_t key_length,
                                       unsigned int offset,
                                       uint64_t *value)
{
  size_t send_length;
  memcached_return rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  unsigned int server_key;

  unlikely (ptr->hosts == NULL || ptr->number_of_hosts == 0)
    return MEMCACHED_NO_SERVERS;

  if ((ptr->flags & MEM_VERIFY_KEY) && (memcachd_key_test((char **)&key, &key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
    return MEMCACHED_BAD_KEY_PROVIDED;

  server_key= memcached_generate_hash(ptr, key, key_length);

  send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                        "%s %s%.*s %u\r\n", verb, 
                        ptr->prefix_key,
                        (int)key_length, key,
                        offset);
  unlikely (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
    return MEMCACHED_WRITE_FAILURE;

  rc= memcached_do(&ptr->hosts[server_key], buffer, send_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  rc= memcached_response(&ptr->hosts[server_key], buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

  /* 
    So why recheck responce? Because the protocol is brain dead :)
    The number returned might end up equaling one of the string 
    values. Less chance of a mistake with strncmp() so we will 
    use it. We still called memcached_response() though since it
    worked its magic for non-blocking IO.
  */
  if (!strncmp(buffer, "ERROR\r\n", 7))
  {
    *value= 0;
    rc= MEMCACHED_PROTOCOL_ERROR;
  }
  else if (!strncmp(buffer, "NOT_FOUND\r\n", 11))
  {
    *value= 0;
    rc= MEMCACHED_NOTFOUND;
  }
  else
  {
    *value= strtoull(buffer, (char **)NULL, 10);
    rc= MEMCACHED_SUCCESS;
  }

  return rc;
}

static memcached_return binary_incr_decr(memcached_st *ptr, uint8_t cmd,
                                         const char *key, size_t key_length,
                                         uint32_t offset, uint64_t *value) 
{
  unsigned int server_key;
#ifndef _MSC_VER
  protocol_binary_request_incr request= {.bytes= {0}};
#else
  protocol_binary_request_incr request= {0};
#endif

  unlikely (ptr->hosts == NULL || ptr->number_of_hosts == 0)
    return MEMCACHED_NO_SERVERS;

  server_key= memcached_generate_hash(ptr, key, key_length);

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= cmd;
  request.message.header.request.keylen= htons((uint16_t)key_length);
  request.message.header.request.extlen= 20;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl(key_length + request.message.header.request.extlen);  
  request.message.body.delta= htonll(offset);
  
  /* TODO: The binary protocol allows you to specify initial and expiry time */
  if ((memcached_do(&ptr->hosts[server_key], request.bytes,
                    sizeof(request.bytes), 0)!=MEMCACHED_SUCCESS) ||
      (memcached_io_write(&ptr->hosts[server_key], key, key_length, 1) == -1)) 
  {
    memcached_io_reset(&ptr->hosts[server_key]);
    return MEMCACHED_WRITE_FAILURE;
  }
 
  return memcached_response(&ptr->hosts[server_key], (char*)value, sizeof(*value), NULL);
}

memcached_return memcached_increment(memcached_st *ptr, 
                                     const char *key, size_t key_length,
                                     uint32_t offset,
                                     uint64_t *value)
{
  memcached_return rc= memcached_validate_key_length(key_length, ptr->flags & MEM_BINARY_PROTOCOL);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_INCREMENT_START();
  if (ptr->flags & MEM_BINARY_PROTOCOL) 
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_INCREMENT, key, 
			 key_length, offset, value);
  else 
     rc= memcached_auto(ptr, "incr", key, key_length, offset, value);
  
  LIBMEMCACHED_MEMCACHED_INCREMENT_END();

  return rc;
}

memcached_return memcached_decrement(memcached_st *ptr, 
                                     const char *key, size_t key_length,
                                     uint32_t offset,
                                     uint64_t *value)
{
  memcached_return rc= memcached_validate_key_length(key_length, ptr->flags & MEM_BINARY_PROTOCOL);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  LIBMEMCACHED_MEMCACHED_DECREMENT_START();
  if (ptr->flags & MEM_BINARY_PROTOCOL) 
    rc= binary_incr_decr(ptr, PROTOCOL_BINARY_CMD_DECREMENT, key, 
			 key_length, offset, value);
  else 
    rc= memcached_auto(ptr, "decr", key, key_length, offset, value);      
  
  LIBMEMCACHED_MEMCACHED_DECREMENT_END();

  return rc;
}
