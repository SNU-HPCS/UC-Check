all: victim

# Configure -falign-functions to allocate func_A/func_B in different sets
victim: ../utils.c victim.c
	gcc -g $^ -o $@ -falign-functions=128
clean:
	rm victim
