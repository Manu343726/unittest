import unittest, unittest.mock
import mynamespace

class ExampleTestCase(unittest.TestCase):

    @unittest.mock.patch('mynamespace.ExampleClass.identity', return_value=42)
    def test_another_one_bites_the_dust(self, identity):
        object = mynamespace.ExampleClass()

        self.assertEqual(object.methodThatCallsIdentity(), 42)
        identity.assert_called_once_with(43)
