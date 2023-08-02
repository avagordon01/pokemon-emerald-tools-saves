# pokemon gen 3 (ruby, sapphire, emerald, fire red, leaf green) saves and tools

this repo contains:
- tool for validating gen 3 saves
- tool for giving mystery gifts / applying wonder cards to gen 3 saves
- script for moving gen 3 saves between lemuroid (android) and mgba (linux) and back again
  - note: to definitely save in-game in lemuroid, you have to same using "start > SAVE" *and* then close lemuroid with "... > Quit"
- script/instructions for duplicating a save and using mgba multiplayer to trade with yourself (in `self-trade.sh`)
- my gen 3 saves

## dependencies

```
sudo pacman -S android-tools mgba-qt
```

## getting the ROM
these tools have only been tested with the (USA, Europe) aka International ROM, the Japanese data structures are known to be different
if it helps, the ROM I use (in both emulators) is:
```
> md5sum "Pokemon - Emerald Version (USA, Europe).gba"
605b89b67018abcea91e693a4dd25be3  Pokemon - Emerald Version (USA, Europe).gba
>
```

- https://pokemonrom.net/pokemon-emerald-rom/
- https://www.emulatorgames.net/roms/gameboy-advance/pokemon-emerald-version/
- https://www.romsgames.net/gameboy-advance-rom-pokemon-emerald-version/

## getting the Wonder Cards

- https://projectpokemon.org/home/forums/topic/39327-gen-iii-event-contribution-thread/
- https://projectpokemon.org/home/files/file/645-mystery-gift-tool-gen-3/
- https://github.com/projectpokemon/EventsGallery/tree/master/Released/Gen%203/ENG/Wondercards

## todo

integrate with the 100% completion guide https://scripterswar.com/pokemon/completion/Emerald#

## notes

this is all based on information from these sources:
- https://bulbapedia.bulbagarden.net/wiki/Save_data_structure_(Generation_III)
- https://projectpokemon.org/home/forums/topic/35903-gen-3-mystery-eventgift-research/

## common problems

saving in lemuroid can be fiddly, especially when pushing saves using `adb push`
to save in lemuroid (such that you can move the file between emulators/tools/...) make sure you do an in-game save, and "quit" lemuroid
if that fails, do `adb shell` and `cd /storage/self/primary/Android/data/com.swordfish.lemuroid/files/saves`, and `chmod 666 *.srm` (this is because when you push from `adb shell` it creates the file under a `shell` user rather than whatever user the app is running as, which stops it from being writable)

you can check if the save was successful by checking the timestamp of the save file with `ls -lh` (in `adb shell`) and making sure it's recent

## mystery gifts

using mystery gifts you can access (/catch): [Faraway Island](https://bulbapedia.bulbagarden.net/wiki/Faraway_Island) ([Mew](https://bulbapedia.bulbagarden.net/wiki/Mew_(Pok%C3%A9mon))), [Naval Rock](https://bulbapedia.bulbagarden.net/wiki/Navel_Rock) ([Lugia](https://bulbapedia.bulbagarden.net/wiki/Lugia_(Pok%C3%A9mon)), [Ho-Oh](https://bulbapedia.bulbagarden.net/wiki/Ho-Oh_(Pok%C3%A9mon))), [Birth Island](https://bulbapedia.bulbagarden.net/wiki/Birth_Island) ([Deoxys](https://bulbapedia.bulbagarden.net/wiki/Deoxys_(Pok%C3%A9mon)))

do `gift-tool save-file mystery-gift-file`

e.g. ```./gift-tool \
  'Pokemon - Emerald Version (USA, Europe).srm' \
  'EventsGallery/Unreleased/Gen 3/ENG/Wondercards/E - Item Old Sea Map (debug)(ENG).wc3'
```

get the mystery gift files from here https://github.com/projectpokemon/EventsGallery/blob/master/Released/Gen%203/ENG/Wondercards/ make sure to choose one that matches your game version E for Emerald, FL for Fire Red or Leaf Green, RS for Ruby or Sapphire (untested)
