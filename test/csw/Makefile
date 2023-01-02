#Assembleur = lwasm
AS		= /usr/local/bin/lwasm

#Flags d'assemblage:
# -9: Langage assembleur pour le 6809
# -l: Produit le fichier de listing
# --format=srec: Le fichier de sortie est au format srec
# -s Genere la liste des symboles
# --map: Génere le fichier de mapping
HEXFLAGS		= -9 -l$@.lst --format=ihex -s --map=$@.map
S19FLAGS		= -9 -l$@.lst --format=srec -s --map=$@.map

all: csw 

csw: csw.S
	$(AS) -o $@.s19 $< $(S19FLAGS)
	$(AS) -o $@.hex $< $(HEXFLAGS)
	srec_cat $@.hex -intel -offset - 0xE000 -o $@.bin -binary

clean:
	rm -rf *.o *.lst *.map *.s19 *.hex *.bin
	
