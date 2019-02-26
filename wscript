# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('bgp', ['core', 'internet', 'network'])
    module.source = [
        'model/bgp-speaker.cc',
        'model/bgp-peer.cc',
        'model/bgp-route.cc',
        'model/bgp-routing.cc',
        'model/bgp-fragment.cc',
        'model/bgp-filter.cc',
        'model/libbgp.cc',
        'model/build.cc',
        'model/parse.cc',
        'model/bgp-peerstatus.cc',
        'helper/bgp-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('bgp')
    module_test.source = [
        'test/bgp-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'bgp'
    headers.source = [
        'model/bgp-speaker.h',
        'model/bgp-peer.h',
        'model/bgp-route.h',
        'model/bgp-routing.h',
        'model/bgp-peerstatus.h',
        'model/bgp-fragment.h',
        'model/bgp-filter.h',
        'model/libbgp.h',
        'helper/bgp-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

