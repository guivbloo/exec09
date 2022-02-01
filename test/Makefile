#Assembleur = lwasm
AS		= /usr/local/bin/lwasm

#Virtual target location
VIRTUAL_TARGET = /home/guivbloo/6809/exec09/exec09.sh

#Flags d'assemblage:
# -9: Langage assembleur pour le 6809
# -l: Produit le fichier de listing
# --format=srec: Le fichier de sortie est au format srec
# -s Genere la liste des symboles
# --map: Génere le fichier de mapping
HEXFLAGS		= -9 -l$*.lst --format=ihex -s --map=$*.map
S19FLAGS		= -9 -l$*.lst --format=srec -s --map=$*.map

helloworld.o: helloworld.S
	$(AS) -o $*.hex $< $(HEXFLAGS)
	$(AS) -o $*.s19 $< $(S19FLAGS)
	objcopy -I ihex -O binary $*.hex $*.bin

clean:
	rm -rf *.o *.lst *.map *.s19 *.hex *.bin
	
