#pragma once

#include "esp_http_server.h"
#include "esp_partition.h"

esp_err_t   ota_handler(httpd_req_t *req);
const char *ota_state_name(const esp_partition_t *partition);