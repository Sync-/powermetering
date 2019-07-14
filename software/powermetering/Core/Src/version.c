#include "version.h"

volatile const version_info_t version_info __attribute__((section(".version_info"))) = {
    .product_name = "powermetering",
    .major        = 0,
    .minor        = 1,
    .patch        = 0};
