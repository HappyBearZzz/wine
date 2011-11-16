/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#include "initguid.h"
#include "d3d10.h"
#include "wine/test.h"

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a HW device\n");
        return device;
    }

    trace("Failed to create a HW device, trying REF\n");
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a REF device\n");
        return device;
    }

    trace("Failed to create a device, returning NULL\n");
    return NULL;
}

static void test_device_interfaces(ID3D10Device *device)
{
    HRESULT hr;
    IUnknown *obj;

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IUnknown, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IUnknown (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement ID3D10Device (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIObject, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIObject (%#x)\n", hr);

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&obj)))
        IUnknown_Release(obj);
    ok(SUCCEEDED(hr), "ID3D10Device does not implement IDXGIDevice (%#x)\n", hr);
}

static void test_stateblock_mask(void)
{
    static const struct
    {
        UINT start_idx;
        UINT count;
        BYTE expected_disable[5];
        BYTE expected_enable[5];
    }
    capture_test[] =
    {
        { 8,  4, {0xff, 0xf0, 0xff, 0xff, 0xff}, {0x00, 0x0f, 0x00, 0x00, 0x00}},
        { 9,  4, {0xff, 0xe1, 0xff, 0xff, 0xff}, {0x00, 0x1e, 0x00, 0x00, 0x00}},
        {10,  4, {0xff, 0xc3, 0xff, 0xff, 0xff}, {0x00, 0x3c, 0x00, 0x00, 0x00}},
        {11,  4, {0xff, 0x87, 0xff, 0xff, 0xff}, {0x00, 0x78, 0x00, 0x00, 0x00}},
        {12,  4, {0xff, 0x0f, 0xff, 0xff, 0xff}, {0x00, 0xf0, 0x00, 0x00, 0x00}},
        {13,  4, {0xff, 0x1f, 0xfe, 0xff, 0xff}, {0x00, 0xe0, 0x01, 0x00, 0x00}},
        {14,  4, {0xff, 0x3f, 0xfc, 0xff, 0xff}, {0x00, 0xc0, 0x03, 0x00, 0x00}},
        {15,  4, {0xff, 0x7f, 0xf8, 0xff, 0xff}, {0x00, 0x80, 0x07, 0x00, 0x00}},
        { 8, 12, {0xff, 0x00, 0xf0, 0xff, 0xff}, {0x00, 0xff, 0x0f, 0x00, 0x00}},
        { 9, 12, {0xff, 0x01, 0xe0, 0xff, 0xff}, {0x00, 0xfe, 0x1f, 0x00, 0x00}},
        {10, 12, {0xff, 0x03, 0xc0, 0xff, 0xff}, {0x00, 0xfc, 0x3f, 0x00, 0x00}},
        {11, 12, {0xff, 0x07, 0x80, 0xff, 0xff}, {0x00, 0xf8, 0x7f, 0x00, 0x00}},
        {12, 12, {0xff, 0x0f, 0x00, 0xff, 0xff}, {0x00, 0xf0, 0xff, 0x00, 0x00}},
        {13, 12, {0xff, 0x1f, 0x00, 0xfe, 0xff}, {0x00, 0xe0, 0xff, 0x01, 0x00}},
        {14, 12, {0xff, 0x3f, 0x00, 0xfc, 0xff}, {0x00, 0xc0, 0xff, 0x03, 0x00}},
        {15, 12, {0xff, 0x7f, 0x00, 0xf8, 0xff}, {0x00, 0x80, 0xff, 0x07, 0x00}},
    };
    D3D10_STATE_BLOCK_MASK mask_x, mask_y, result;
    HRESULT hr;
    UINT i;

    memset(&mask_x, 0, sizeof(mask_x));
    memset(&mask_y, 0, sizeof(mask_y));
    memset(&result, 0, sizeof(result));

    mask_x.VS = 0x33;
    mask_y.VS = 0x55;
    mask_x.Predication = 0x99;
    mask_y.Predication = 0xaa;

    hr = D3D10StateBlockMaskDifference(&mask_x, &mask_y, &result);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskDifference failed, hr %#x.\n", hr);
    ok(result.VS == 0x66, "Got unexpected result.VS %#x.\n", result.VS);
    ok(result.Predication == 0x33, "Got unexpected result.Predication %#x.\n", result.Predication);
    hr = D3D10StateBlockMaskDifference(NULL, &mask_y, &result);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDifference(&mask_x, NULL, &result);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDifference(&mask_x, &mask_y, NULL);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);

    memset(&result, 0xff, sizeof(result));
    hr = D3D10StateBlockMaskDisableAll(&result);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskDisableAll failed, hr %#x.\n", hr);
    ok(!result.VS, "Got unexpected result.VS %#x.\n", result.VS);
    ok(!result.Predication, "Got unexpected result.Predication %#x.\n", result.Predication);
    hr = D3D10StateBlockMaskDisableAll(NULL);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);

    memset(&result, 0, sizeof(result));
    hr = D3D10StateBlockMaskEnableAll(&result);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskEnableAll failed, hr %#x.\n", hr);
    ok(result.VS == 0xff, "Got unexpected result.VS %#x.\n", result.VS);
    ok(result.Predication == 0xff, "Got unexpected result.Predication %#x.\n", result.Predication);
    hr = D3D10StateBlockMaskEnableAll(NULL);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);

    result.VS = 0xff;
    hr = D3D10StateBlockMaskDisableCapture(&result, D3D10_DST_VS, 0, 1);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskDisableCapture failed, hr %#x.\n", hr);
    ok(result.VS == 0xfe, "Got unexpected result.VS %#x.\n", result.VS);
    hr = D3D10StateBlockMaskDisableCapture(&result, D3D10_DST_VS, 0, 4);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDisableCapture(&result, D3D10_DST_VS, 1, 1);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskDisableCapture(NULL, D3D10_DST_VS, 0, 1);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    result.VS = 0;
    hr = D3D10StateBlockMaskEnableCapture(&result, D3D10_DST_VS, 0, 1);
    ok(SUCCEEDED(hr), "D3D10StateBlockMaskEnableCapture failed, hr %#x.\n", hr);
    ok(result.VS == 0x01, "Got unexpected result.VS %#x.\n", result.VS);
    hr = D3D10StateBlockMaskEnableCapture(&result, D3D10_DST_VS, 0, 4);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskEnableCapture(&result, D3D10_DST_VS, 1, 1);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    hr = D3D10StateBlockMaskEnableCapture(NULL, D3D10_DST_VS, 0, 1);
    ok(hr == E_INVALIDARG, "Got unexpect hr %#x.\n", hr);
    for (i = 0; i < sizeof(capture_test) / sizeof(*capture_test); ++i)
    {
        memset(&result, 0xff, sizeof(result));
        hr = D3D10StateBlockMaskDisableCapture(&result, D3D10_DST_VS_SHADER_RESOURCES,
                capture_test[i].start_idx, capture_test[i].count);
        ok(SUCCEEDED(hr), "D3D10StateBlockMaskDisableCapture failed, hr %#x.\n", hr);

        ok(!memcmp(result.VSShaderResources, capture_test[i].expected_disable, 5),
                "Got unexpect result.VSShaderResources[0..4] {%#x, %#x, %#x, %#x, %#x} for test %u.\n",
                result.VSShaderResources[0], result.VSShaderResources[1],
                result.VSShaderResources[2], result.VSShaderResources[3],
                result.VSShaderResources[4], i);

        memset(&result, 0, sizeof(result));
        hr = D3D10StateBlockMaskEnableCapture(&result, D3D10_DST_VS_SHADER_RESOURCES,
                capture_test[i].start_idx, capture_test[i].count);
        ok(SUCCEEDED(hr), "D3D10StateBlockMaskEnableCapture failed, hr %#x.\n", hr);

        ok(!memcmp(result.VSShaderResources, capture_test[i].expected_enable, 5),
                "Got unexpect result.VSShaderResources[0..4] {%#x, %#x, %#x, %#x, %#x} for test %u.\n",
                result.VSShaderResources[0], result.VSShaderResources[1],
                result.VSShaderResources[2], result.VSShaderResources[3],
                result.VSShaderResources[4], i);
    }
}

START_TEST(device)
{
    ID3D10Device *device;
    ULONG refcount;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests\n");
        return;
    }

    test_device_interfaces(device);
    test_stateblock_mask();

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
