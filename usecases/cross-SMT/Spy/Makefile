all: spy

spy: ../utils.c codegen.c spy.c
	gcc -O0 -g $^ -falign-functions=256 -o $@

clean:
	rm spy
