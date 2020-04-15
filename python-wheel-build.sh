#!/bin/sh

cd /src/python
/opt/python/cp37-cp37m/bin/python setup.py bdist_wheel
auditwheel -v repair ./dist/*linux_x86_64.whl
rm ./dist/*linux_x86_64.whl
cp wheelhouse/*.whl dist
