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
        'model/nlri.cc',
        'model/libbgp/src/libbgp.cc',
        'model/libbgp/src/build.cc',
        'model/libbgp/src/parse.cc',
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
        'model/nlri.h',
        'helper/bgp-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

