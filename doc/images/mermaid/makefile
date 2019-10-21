
SRCS = $(wildcard *.mmd)
PNGS = $(SRCS:%.mmd=../%.png)


all: node_modules $(PNGS)


node_modules:
	yarn init --yes
	yarn add mermaid.cli



../%.png:  %.mmd style.css makefile 
	./node_modules/.bin/mmdc -w 1024 --cssFile style.css -i $< -o $@