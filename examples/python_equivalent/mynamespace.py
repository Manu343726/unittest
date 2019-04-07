
class ExampleClass:

    def identity(self, value):
        return value

    def methodThatCallsIdentity(self):
        return self.identity(42)
