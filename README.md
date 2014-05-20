=== ABOUT ===
Programmatically compiling all shaders the first time an application is run and saving the binaries for reuse can significantly reduce load times for games in subsequent runs. The OpenGL* ES 3.0 sample code introduced here demonstrates a simple implementation of this capability.

=== INSTALLATION ===
Navigate to PrecompiledShaders\projects\android with a device connected, or the emulator running.
Run the following:
	android update project –p .
	ndk-build
	ant debug
	ant installd