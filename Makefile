all:
	clang++ -std=c++17 -I include src/main.cpp src/debugger_console.cpp src/scan_console.cpp src/util.cpp -o analyst
	codesign --entitlements Entitlements.plist -f -s - analyst

clean:
	rm -f analyst
