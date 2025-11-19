all:
	g++ -std=c++20 -g main.cc sam.cc tree.cc graph.cc -o sam -Wfatal-errors
