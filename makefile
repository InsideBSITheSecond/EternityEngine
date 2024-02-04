CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SHADERS = shaders/shader.frag shaders/shader.vert

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS) -Iincludes
	
shaders: $(SHADERS)
	cd shaders && ./compile_shaders.sh

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest