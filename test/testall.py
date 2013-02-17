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

suite = unittest.defaultTestLoader.loadTestsFromNames(test_names)
unittest.TextTestRunner().run(suite)
