{
  'TOOLS': ['pnacl'],
  'TARGETS': [
    {
      'NAME' : 'life',
      'TYPE' : 'main',
      'SOURCES' : [
        'life.cc',
      ],
      'DEPS': ['ppapi_simple', 'nacl_io'],
      'LIBS': ['ppapi_simple', 'nacl_io', 'sdk_util', 'ppapi_cpp', 'ppapi', 'pthread']
    }
  ],
  'DATA': [
    'example.js'
  ],
  'DEST': 'examples/demo',
  'NAME': 'life_simd',
  'TITLE': "Conway's Life",
  'GROUP': 'Demo'
}
