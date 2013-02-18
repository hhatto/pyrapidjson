import glob
import os
import sys
import unittest


test_root = os.path.dirname(os.path.abspath(__file__))
test_files = glob.glob(os.path.join(test_root, 'test_*.py'))

os.chdir(test_root)
sys.path.insert(0, os.path.dirname(test_root))
sys.path.insert(0, test_root)
test_names = [os.path.basename(name)[:-3] for name in test_files]

if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromNames(test_names)
    ret = unittest.TextTestRunner().run(suite)
    retcode = 0
    if ret.failures or ret.errors:
        retcode = 1
    sys.exit(retcode)
