/*
* The MIT License (MIT)

* Copyright (c) 2021-2023 Krux contributors

* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"

double log2(double n) {
    // Constants for a simple logarithm approximation
    static const double inv_log2 = 1.4426950408889634;
    return log(n) * inv_log2;
}

STATIC mp_obj_t image_entropy_16b(mp_obj_t image_bytes) {
    mp_buffer_info_t image_bytes_buffer;
    mp_get_buffer_raise(image_bytes, &image_bytes_buffer, MP_BUFFER_READ);

    // uint8_t image_buffer[320*240*2];
    uint8_t *image_buffer = malloc(320 * 240 * 2 * sizeof(uint8_t));
    if (image_buffer == NULL) {
        mp_raise_OSError("Not enough memory");  // MP_ENOMEM not defined
    }
    memcpy(image_buffer, image_bytes_buffer.buf, image_bytes_buffer.len);

    // uint16_t pixel_counts[65536] = {0};
    uint16_t *pixel_counts = malloc(65536 * sizeof(uint16_t));
    if (pixel_counts == NULL) {
        free(image_buffer);
        mp_raise_OSError("Not enough memory");
    }
    memset(pixel_counts, 0, 65536 * sizeof(uint16_t));

    // Maximum pixel count = 76800 (320*240) in QVGA mode
    // So it's possible that, one, but only one count is greater than uint16_t
    uint32_t long_pixel_count = 0;

    for (size_t i = 0; i < image_bytes_buffer.len; i += 2) {
        uint16_t pixel_value = image_buffer[i] + (image_buffer[i + 1] << 8);
        if (pixel_counts[pixel_value] < 0xFFFE) {
            pixel_counts[pixel_value]++;
        } else if (pixel_counts[pixel_value] < 0xFFFF) {
            // If uint16_t is full
            pixel_counts[pixel_value]++;  // The 0xFFFF will also work as a flag
            long_pixel_count = 0xFFFF;
        } else {
            // Will increment uint32_t long count instead of pixel_counts[pixel_value](which is full)
            long_pixel_count++;
        }
    }

    free(image_buffer);
    size_t total_pixels = image_bytes_buffer.len / 2;
    double entropy = 0;


    for (int i = 0; i < 65536; ++i) {
        // mp_printf(&mp_plat_print, "%u\n", (unsigned int)pixel_counts[i]);
        if (pixel_counts[i] > 0) {
            double probability = 0;
            if (pixel_counts[i] < 0xFFFF) {
                probability = (double)pixel_counts[i] / total_pixels;
            } else {
                probability = (double)long_pixel_count / total_pixels;
            }
            if (probability > 0) entropy -= probability * log2(probability);
        }
    }
    free(pixel_counts);

    mp_float_t entropy_mp = (mp_float_t)entropy;
    return mp_obj_new_float(entropy_mp);
}

MP_DEFINE_CONST_FUN_OBJ_1(shannon_func_img16b_obj, image_entropy_16b);


/****************************** MODULE ******************************/
STATIC const mp_map_elem_t shannon_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_shannon) },
    {MP_OBJ_NEW_QSTR(MP_QSTR_entropy_img16b), (mp_obj_t)&shannon_func_img16b_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_shannon_globals_dict, shannon_globals_table);

// Define module object.
const mp_obj_module_t shannon_module = {
    .base = {&mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_shannon_globals_dict,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_shannon, shannon_module, MODULE_SHANNON_ENABLED);
