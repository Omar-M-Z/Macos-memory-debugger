all:
	clang++ -std=c++17 -I include src/main.cpp src/debugger_console.cpp src/metal_resources.cpp src/scan_commands.cpp src/util.cpp -framework Foundation -framework QuartzCore -framework Metal -framework MetalKit -o analyst
	codesign --entitlements Entitlements.plist -f -s - analyst

clean:
	rm -f analyst
