#include "presets.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

#include "params.h"

namespace df {

// Every preset is "key=value;..." applied on top of the defaults. Keys are parameter ids (see the User Guide),
// groove.pattern selects the demo groove that suits the kit (0 Rock Basic ... 27 Indie Dance).
static const Preset kPresets[] = {
    // ------------------------------------------------------------------ ROCK / ACOUSTIC
    {"Studio Rock", "Rock",
     "kick.tune=55;kick.decay=380;kick.punch=50;kick.click=55;kick.sub=30;"
     "snare.tune=190;snare.wires=60;snare.body=75;snare.snap=50;snare.shell=45;"
     "comp.mix=35;comp.thr=-14;comp.ratio=3.5;comp.att=25;comp.rel=120;eq.hi=1.5;"
     "room.type=0;room.mix=20;room.size=45;room.decay=900;groove.pattern=0;snare.buzz=25;snare.tattack=15"},
    {"Big Room Rock", "Rock",
     "kick.tune=50;kick.decay=520;kick.sub=45;kick.click=50;"
     "snare.tune=170;snare.body=85;snare.bodydecay=220;snare.wiredecay=380;snare.snap=60;snare.rimshot=30;"
     "tomf.tune=80;tomf.decay=900;tomm.tune=112;tomm.decay=760;tomh.tune=160;tomh.decay=620;"
     "crash.decay=3200;crash.wash=55;"
     "room.type=2;room.mix=38;room.size=70;room.decay=2200;room.pre=18;comp.mix=45;comp.thr=-16;comp.att=30;groove.pattern=3"},
    {"Stadium Thunder", "Rock",
     "kick.tune=46;kick.sweep=2.2;kick.decay=650;kick.sub=55;kick.punch=60;"
     "snare.tune=160;snare.bodydecay=260;snare.body=90;snare.wiredecay=420;snare.snap=70;snare.rimshot=45;snare.shell=60;"
     "tomf.tune=70;tomf.decay=1200;tomm.tune=100;tomm.decay=1000;tomh.tune=140;tomh.decay=800;"
     "crash.decay=3800;ride.decay=3600;room.type=2;room.mix=48;room.size=85;room.decay=3200;room.pre=25;"
     "sat.drive=15;comp.mix=50;comp.thr=-18;comp.ratio=4;groove.pattern=3;snare.buzz=30;snare.tattack=20"},
    {"Garage Punk", "Rock",
     "humanize=85;kick.tune=60;kick.decay=260;kick.click=65;kick.punch=60;"
     "snare.tune=225;snare.tension=70;snare.bodydecay=120;snare.wires=80;snare.wiredecay=210;snare.snap=75;snare.rimshot=35;"
     "hat.bright=13000;hat.closeddecay=70;hat.metal=75;sat.drive=40;sat.mix=80;eq.hi=2;eq.lo=-1;"
     "comp.mix=55;comp.ratio=6;comp.att=6;room.mix=10;room.type=1;groove.pattern=1"},
    {"Grunge Wall", "Rock",
     "kick.tune=52;kick.decay=450;kick.sub=40;kick.click=40;"
     "snare.tune=175;snare.body=90;snare.bodydecay=240;snare.wires=50;snare.shell=70;snare.snap=40;"
     "tomf.tune=78;tomm.tune=108;tomh.tune=150;hat.bright=9000;hat.metal=50;crash.bright=9000;crash.trash=30;"
     "room.type=2;room.mix=36;room.size=65;room.decay=1800;room.damp=3800;sat.drive=30;comp.mix=60;comp.ratio=5;groove.pattern=3"},
    {"Vintage 60s", "Vintage",
     "kick.tune=68;kick.sweep=1.0;kick.decay=300;kick.punch=20;kick.click=15;kick.sub=10;kick.damp=4500;"
     "snare.tune=230;snare.tension=20;snare.wires=45;snare.snap=25;snare.damp=7000;snare.bodydecay=150;"
     "tomf.tune=110;tomm.tune=150;tomh.tune=210;tomf.reso=70;tomm.reso=70;tomh.reso=70;"
     "hat.bright=8000;hat.metal=40;hat.tick=20;crash.bright=8500;ride.bright=7500;ride.bell=45;"
     "eq.hi=-4;eq.lo=2;sat.drive=25;comp.mix=30;room.type=1;room.mix=12;width=70;humanize=40;groove.pattern=4;snare.buzz=40;kick.tsustain=-25"},
    {"Soul Ballad Warm", "Vintage",
     "kick.tune=62;kick.sweep=1.2;kick.decay=280;kick.damp=3500;kick.click=20;kick.sub=30;"
     "snare.tune=200;snare.tension=10;snare.body=85;snare.wires=40;snare.damp=6000;snare.snap=25;"
     "hat.bright=7500;hat.metal=35;hat.tick=15;hat.level=-3;eq.hi=-3;eq.lo=2;sat.drive=20;"
     "room.type=0;room.mix=22;room.decay=1100;room.damp=4500;comp.mix=25;groove.pattern=7"},
    {"Dry Jazz Kit", "Jazz",
     "kick.tune=70;kick.sweep=0.8;kick.decay=220;kick.click=10;kick.sub=5;kick.damp=6000;kick.dyn=95;"
     "snare.tune=250;snare.tension=40;snare.wires=35;snare.wiredecay=180;snare.body=60;snare.snap=10;snare.dyn=95;"
     "tomf.tune=100;tomm.tune=140;tomh.tune=200;"
     "hat.closeddecay=70;hat.bright=9000;hat.metal=35;ride.ping=70;ride.bell=45;ride.decay=3500;ride.wash=35;ride.dirt=0;ride.level=2;"
     "room.type=2;room.mix=14;room.size=50;humanize=60;comp.mix=0;groove.pattern=20;snare.buzz=45;kick.tsustain=-20"},
    {"Funk Pocket Dry", "Funk",
     "kick.tune=58;kick.decay=240;kick.punch=60;snare.tune=205;snare.tension=45;snare.wires=70;snare.wiredecay=170;"
     "snare.snap=55;snare.shell=30;snare.dyn=100;hat.closeddecay=40;hat.metal=55;"
     "room.mix=8;room.type=1;comp.mix=55;comp.ratio=5;comp.att=8;comp.rel=90;humanize=30;groove.pattern=5;snare.tattack=30;kick.tsustain=-40;snare.tsustain=-20"},
    {"Pop Tight", "Pop",
     "kick.tune=60;kick.decay=300;kick.punch=55;kick.click=55;"
     "snare.tune=215;snare.tension=40;snare.bodydecay=140;snare.wires=70;snare.wiredecay=200;snare.snap=60;"
     "hat.bright=12500;hat.tick=55;eq.hi=2;comp.mix=45;comp.ratio=4;room.type=5;room.mix=14;room.size=35;groove.pattern=2"},
    {"Disco Strut", "Dance",
     "kick.tune=58;kick.decay=320;kick.punch=50;kick.click=45;snare.tune=210;snare.wires=75;snare.snap=60;snare.wiredecay=230;"
     "hat.bright=13500;hat.opendecay=550;hat.metal=70;hat.tick=60;crash.decay=2000;"
     "comp.mix=40;room.type=0;room.mix=12;eq.hi=2.5;groove.pattern=19"},
    {"Indie Dance", "Pop",
     "kick.tune=56;kick.decay=330;kick.click=60;kick.sub=40;snare.tune=200;snare.tension=35;snare.wires=70;snare.snap=60;"
     "hat.metal=70;hat.char=40;hat.bright=12000;perc.tone=70;room.type=5;room.mix=20;room.size=45;sat.drive=15;"
     "comp.mix=50;comp.ratio=4;groove.pattern=27"},

    // ------------------------------------------------------------------ METAL
    {"Death Metal Trigger", "Metal",
     "kick.tune=62;kick.sweep=1.2;kick.sweeptime=14;kick.decay=150;kick.punch=90;kick.click=90;kick.clicktone=4800;kick.sub=25;kick.damp=18000;"
     "snare.tune=240;snare.tension=70;snare.bodydecay=90;snare.body=55;snare.wires=55;snare.wiredecay=130;snare.wiretone=6500;snare.snap=90;snare.rimshot=55;snare.shell=20;"
     "tomf.decay=420;tomm.decay=360;tomh.decay=300;tomf.tune=88;tomm.tune=125;tomh.tune=180;tomf.attack=85;tomm.attack=85;tomh.attack=85;"
     "hat.bright=14000;hat.tick=70;crash.trash=35;crash.decay=2400;"
     "eq.lo=-1;eq.mid=-3;eq.midf=500;eq.hi=3;comp.mix=60;comp.ratio=6;comp.thr=-16;comp.att=8;sat.drive=20;"
     "room.mix=8;room.type=1;humanize=10;groove.pattern=11;kick.tattack=55;snare.tattack=40;kick.tsustain=-30"},
    {"Thrash Clank", "Metal",
     "kick.tune=60;kick.decay=200;kick.click=80;kick.clicktone=3000;kick.punch=70;"
     "snare.tune=280;snare.tension=80;snare.body=40;snare.wires=85;snare.wiredecay=200;snare.snap=70;snare.rimshot=60;"
     "crash.tune=2;crash.trash=25;crash.decay=2400;ride.dirt=25;eq.mid=3;eq.midf=2000;sat.drive=30;"
     "comp.mix=55;comp.ratio=6;room.mix=10;room.type=1;groove.pattern=9"},
    {"Black Metal Rawness", "Metal",
     "kick.tune=58;kick.decay=140;kick.click=100;kick.clicktone=6000;kick.sub=0;kick.damp=20000;kick.punch=60;"
     "snare.tune=300;snare.tension=90;snare.bodydecay=70;snare.body=30;snare.wires=100;snare.wiredecay=90;snare.wiretone=8000;snare.snap=100;"
     "hat.tune=3;hat.bright=16000;hat.closeddecay=30;crash.tune=3;crash.decay=4000;crash.wash=70;crash.trash=50;crash.bright=16000;ride.dirt=40;"
     "eq.lo=-4;eq.hi=4;sat.drive=55;sat.mix=70;room.type=5;room.mix=25;room.damp=5000;room.decay=1600;width=70;groove.pattern=10"},
    {"Djent Surgical", "Metal",
     "kick.tune=54;kick.decay=120;kick.click=70;kick.sub=30;kick.punch=100;kick.clicktone=3800;"
     "snare.tune=220;snare.tension=60;snare.bodydecay=80;snare.wires=50;snare.wiredecay=100;snare.snap=80;"
     "tomf.decay=300;tomm.decay=260;tomh.decay=220;room.mix=0;comp.mix=70;comp.ratio=8;comp.att=3;comp.thr=-18;"
     "eq.mid=-4;eq.midf=400;lim.ceil=-0.5;hat.tick=70;groove.pattern=13"},
    {"Doom Cavern", "Metal",
     "kick.tune=42;kick.sweep=1.0;kick.sweeptime=40;kick.decay=900;kick.sub=70;kick.click=20;kick.punch=30;"
     "snare.tune=150;snare.bodydecay=400;snare.wiredecay=600;snare.tension=10;snare.body=90;"
     "tomf.tune=62;tomf.decay=1600;tomm.tune=88;tomm.decay=1400;tomh.tune=125;tomh.decay=1200;"
     "crash.decay=5000;crash.bloom=200;ride.decay=5000;room.type=3;room.mix=45;room.decay=5000;room.size=90;room.pre=30;room.damp=3500;"
     "sat.drive=25;comp.mix=40;groove.pattern=12;snare.tsustain=30"},
    {"Grind Machine", "Metal",
     "kick.tune=66;kick.decay=110;kick.click=85;kick.punch=85;kick.damp=12000;"
     "snare.tune=260;snare.tension=75;snare.bodydecay=70;snare.wiredecay=100;snare.snap=85;"
     "hat.tick=75;crash.decay=1800;sat.drive=40;comp.mix=65;comp.ratio=10;comp.att=2;room.mix=5;room.type=1;groove.pattern=10"},
    {"Heavy Gallop", "Metal",
     "kick.tune=56;kick.decay=260;kick.click=60;kick.punch=60;"
     "snare.tune=200;snare.wires=70;snare.snap=60;snare.shell=50;snare.bodydecay=170;"
     "crash.decay=2800;ride.ping=65;room.type=2;room.mix=25;room.decay=1600;comp.mix=45;comp.ratio=5;sat.drive=15;groove.pattern=8"},

    // ------------------------------------------------------------------ ELECTRONIC
    {"808 Sub Heavy", "Electronic",
     "kick.tune=44;kick.sweep=1.0;kick.sweeptime=60;kick.decay=1400;kick.punch=20;kick.click=10;kick.sub=80;kick.damp=3000;"
     "snare.tune=180;snare.tension=20;snare.body=50;snare.bodydecay=120;snare.wires=50;snare.wiredecay=220;snare.snap=30;snare.shell=0;snare.damp=10000;"
     "tomf.tune=75;tomf.drop=80;tomf.decay=800;tomf.tone=0;tomf.attack=10;tomf.reso=0;"
     "tomm.tune=105;tomm.drop=80;tomm.decay=700;tomm.tone=0;tomm.attack=10;tomm.reso=0;"
     "tomh.tune=150;tomh.drop=80;tomh.decay=600;tomh.tone=0;tomh.attack=10;tomh.reso=0;"
     "hat.metal=90;hat.char=100;hat.bright=10000;hat.closeddecay=35;hat.tick=10;hat.ring=20;"
     "room.type=5;room.mix=15;room.size=40;sat.drive=20;groove.pattern=16"},
    {"909 Classic", "Electronic",
     "kick.tune=55;kick.sweep=2.5;kick.sweeptime=20;kick.decay=450;kick.punch=70;kick.click=55;kick.sub=20;kick.damp=9000;"
     "snare.tune=220;snare.tension=50;snare.body=55;snare.bodydecay=120;snare.wires=80;snare.wiredecay=200;snare.wiretone=5500;snare.snap=70;snare.shell=0;"
     "hat.metal=85;hat.char=70;hat.bright=13000;hat.tick=60;hat.ring=30;comp.mix=40;sat.drive=20;"
     "room.type=5;room.mix=10;groove.pattern=14"},
    {"Techno Warehouse", "Electronic",
     "kick.tune=50;kick.decay=380;kick.punch=90;kick.click=40;kick.sub=55;kick.sweep=2.2;"
     "snare.tune=200;snare.wires=70;snare.wiredecay=150;snare.bodydecay=100;"
     "hat.opendecay=500;hat.metal=80;hat.char=50;room.type=2;room.mix=30;room.decay=1800;room.pre=12;room.damp=4500;"
     "eq.hi=-2;comp.mix=70;comp.ratio=6;comp.att=20;sat.drive=35;groove.pattern=15"},
    {"Synthwave Gated", "Electronic",
     "kick.tune=58;kick.decay=300;kick.punch=60;kick.click=50;kick.sub=40;"
     "snare.tune=190;snare.body=70;snare.bodydecay=170;snare.wires=85;snare.wiredecay=300;snare.snap=65;snare.shell=60;snare.rimshot=25;"
     "tomf.tune=85;tomf.drop=60;tomf.droptime=60;tomf.decay=700;tomf.attack=70;"
     "tomm.tune=120;tomm.drop=60;tomm.droptime=60;tomm.decay=620;tomm.attack=70;"
     "tomh.tune=170;tomh.drop=60;tomh.droptime=60;tomh.decay=520;tomh.attack=70;"
     "room.type=4;room.mix=45;room.gate=260;room.size=65;room.decay=1600;room.pre=0;sat.drive=15;comp.mix=30;groove.pattern=2"},
    {"DnB Rollers", "Electronic",
     "kick.tune=56;kick.decay=240;kick.punch=70;kick.click=60;snare.tune=230;snare.body=80;snare.bodydecay=130;snare.wires=70;"
     "snare.wiredecay=180;snare.snap=85;snare.shell=40;hat.bright=13000;comp.mix=60;comp.ratio=5;room.type=0;room.mix=10;groove.pattern=17"},
    {"IDM Tuned Glitch", "Experimental",
     "kick.tune=70;kick.sweep=3;kick.sweeptime=10;kick.decay=140;"
     "snare.tune=330;snare.tension=100;snare.bodydecay=50;snare.body=100;snare.wires=20;"
     "tomf.tune=98;tomf.decay=220;tomf.drop=0;tomf.attack=30;tomf.reso=0;"
     "tomm.tune=131;tomm.decay=220;tomm.drop=0;tomm.attack=30;tomm.reso=0;"
     "tomh.tune=196;tomh.decay=220;tomh.drop=0;tomh.attack=30;tomh.reso=0;"
     "hat.tune=5;perc.tune=4;room.type=5;room.mix=20;room.size=30;groove.pattern=6"},

    // ------------------------------------------------------------------ HIP-HOP
    {"Trap Sub Kit", "Hip-Hop",
     "kick.tune=40;kick.sweep=0.6;kick.sweeptime=90;kick.decay=1800;kick.sub=90;kick.click=8;kick.damp=2500;"
     "snare.tune=240;snare.tension=80;snare.body=20;snare.bodydecay=70;snare.wires=90;snare.wiredecay=220;snare.wiretone=5000;snare.snap=70;"
     "hat.metal=70;hat.char=60;hat.bright=15000;hat.closeddecay=28;hat.tick=50;sat.drive=25;comp.mix=35;room.type=5;room.mix=14;groove.pattern=16;kick.tattack=-10;hat.tattack=25"},
    {"Boom Bap Dusty", "Hip-Hop",
     "kick.tune=50;kick.decay=340;kick.click=30;kick.punch=50;kick.sub=50;kick.damp=6000;"
     "snare.tune=205;snare.tension=35;snare.body=80;snare.wires=55;snare.snap=55;snare.damp=9000;"
     "hat.bright=9000;hat.closeddecay=50;sat.drive=30;comp.mix=60;comp.ratio=6;room.mix=14;humanize=45;groove.pattern=18;kick.tsustain=-10;snare.buzz=30"},
    {"Lo-Fi Dust", "Hip-Hop",
     "kick.tune=52;kick.decay=300;kick.click=15;kick.damp=2500;kick.sub=40;"
     "snare.tune=190;snare.tension=15;snare.wires=50;snare.snap=20;snare.damp=4500;"
     "hat.bright=6000;hat.metal=40;sat.drive=50;eq.hi=-6;eq.lo=3;comp.mix=80;comp.ratio=8;comp.att=5;"
     "room.mix=12;room.damp=3000;width=60;humanize=70;groove.pattern=26"},

    // ------------------------------------------------------------------ CINEMATIC / INDUSTRIAL
    {"Taiko Hall", "Cinematic",
     "tomf.tune=70;tomf.decay=1800;tomf.drop=20;tomf.droptime=80;tomf.attack=80;tomf.tone=70;tomf.reso=70;"
     "tomm.tune=98;tomm.decay=1500;tomm.drop=20;tomm.droptime=80;tomm.attack=80;tomm.tone=70;tomm.reso=70;"
     "tomh.tune=140;tomh.decay=1200;tomh.drop=20;tomh.droptime=80;tomh.attack=80;tomh.tone=70;tomh.reso=70;"
     "kick.tune=45;kick.decay=900;kick.sub=60;room.type=3;room.mix=40;room.decay=4200;room.pre=35;room.size=85;groove.pattern=24;tomf.tattack=20;tomm.tattack=20"},
    {"Trailer Hits", "Cinematic",
     "kick.tune=38;kick.sweep=1.5;kick.sweeptime=50;kick.decay=1500;kick.sub=80;kick.click=30;"
     "snare.tune=120;snare.bodydecay=500;snare.wiredecay=800;snare.wires=40;snare.body=100;snare.snap=40;"
     "crash.decay=6000;crash.bloom=150;crash.wash=80;crash.trash=20;crash.tune=-4;"
     "room.type=3;room.mix=55;room.decay=6500;room.size=95;room.pre=30;comp.mix=50;sat.drive=20;groove.pattern=24"},
    {"Apocalypse Toms", "Cinematic",
     "tomf.tune=55;tomf.decay=2200;tomf.drop=50;tomf.reso=60;tomm.tune=75;tomm.decay=2000;tomm.drop=50;tomm.reso=60;"
     "tomh.tune=105;tomh.decay=1800;tomh.drop=50;tomh.reso=60;kick.tune=40;kick.decay=1100;kick.sub=75;"
     "room.type=3;room.mix=50;room.decay=4500;room.size=90;sat.drive=25;groove.pattern=23"},
    {"Industrial Foundry", "Industrial",
     "kick.tune=48;kick.decay=300;kick.punch=100;kick.click=80;kick.drive=60;"
     "snare.tune=260;snare.tension=100;snare.bodydecay=70;snare.wires=90;snare.snap=100;snare.rimshot=70;snare.drive=50;"
     "hat.metal=100;hat.char=80;hat.bright=14000;perc.tone=80;perc.decay=250;sat.drive=60;comp.mix=70;comp.ratio=10;"
     "room.type=5;room.size=40;room.decay=1400;room.mix=25;groove.pattern=25"},

    // ------------------------------------------------------------------ WORLD
    {"Tribal Skins", "World",
     "tomf.tune=100;tomm.tune=150;tomh.tune=220;tomf.decay=700;tomm.decay=620;tomh.decay=540;"
     "tomf.drop=15;tomm.drop=15;tomh.drop=15;tomf.attack=60;tomm.attack=60;tomh.attack=60;tomf.tone=75;tomm.tone=75;tomh.tone=75;"
     "tomf.reso=20;tomm.reso=20;tomh.reso=20;kick.tune=62;kick.decay=300;kick.sub=30;room.type=0;room.mix=18;room.decay=1200;groove.pattern=23"},
    {"Latin Pocket", "Latin",
     "kick.tune=64;kick.decay=250;rim.tone=70;rim.tune=1300;rim.level=3;tomf.tune=110;tomm.tune=160;tomh.tune=230;"
     "hat.bright=9500;perc.tone=65;comp.mix=25;room.type=0;room.mix=14;groove.pattern=21"},
    {"Reggae One-Drop", "Reggae",
     "kick.tune=56;kick.decay=400;kick.sub=55;kick.click=20;rim.tune=1000;rim.decay=90;rim.level=2;"
     "snare.tune=215;snare.tension=30;snare.body=70;hat.bright=9000;room.type=0;room.mix=20;sat.drive=15;groove.pattern=22"},

    // ------------------------------------------------------------------ EXPERIMENTAL
    {"Metal Bin", "Experimental",
     "snare.tune=380;snare.tension=100;snare.bodydecay=40;snare.body=100;snare.wires=0;snare.shell=100;snare.rimshot=100;snare.snap=100;"
     "kick.tune=90;kick.sweep=0.5;kick.decay=80;kick.click=100;hat.tune=-6;hat.metal=100;hat.ring=100;crash.tune=-6;crash.trash=60;"
     "room.type=5;room.mix=25;sat.drive=30;groove.pattern=25"},
    {"Cathedral Kit", "Ambient",
     "kick.decay=700;snare.bodydecay=300;snare.wiredecay=700;tomf.decay=1500;tomm.decay=1300;tomh.decay=1100;"
     "room.type=3;room.mix=70;room.size=100;room.decay=7000;room.pre=40;room.damp=5000;room.width=100;width=130;groove.pattern=24"},
    {"Underwater", "Experimental",
     "kick.damp=700;snare.damp=1200;tomf.damp=900;tomm.damp=900;tomh.damp=900;hat.bright=4000;crash.bright=4000;ride.bright=4000;rim.tone=0;"
     "room.type=5;room.mix=40;room.damp=2000;room.decay=3000;width=140;sat.drive=15;groove.pattern=7"},
    {"Rubber Room", "Experimental",
     "tomf.tune=100;tomf.drop=100;tomf.droptime=140;tomf.decay=600;tomm.tune=140;tomm.drop=100;tomm.droptime=140;tomm.decay=600;"
     "tomh.tune=200;tomh.drop=100;tomh.droptime=140;tomh.decay=600;kick.tune=40;kick.sweep=3.5;kick.sweeptime=100;kick.decay=500;"
     "snare.tension=0;snare.tune=160;snare.bodydecay=300;room.type=1;room.mix=35;groove.pattern=23"},
    {"Broken Radio", "Lo-Fi",
     "kick.drive=60;snare.drive=60;hat.drive=60;crash.drive=60;ride.drive=60;tomf.drive=60;tomm.drive=60;tomh.drive=60;"
     "eq.lo=-9;eq.mid=6;eq.midf=1500;eq.hi=-8;sat.drive=60;comp.mix=80;comp.ratio=10;room.mix=10;room.damp=2500;width=30;humanize=60;groove.pattern=26"},
};

int presetCount() { return (int)(sizeof(kPresets) / sizeof(kPresets[0])); }
const Preset& preset(int i) { return kPresets[i < 0 ? 0 : (i >= presetCount() ? presetCount() - 1 : i)]; }

int presetParse(const char* spec, float* values)
{
    const ParamTable& t = paramTable();
    int unknown = 0;
    const char* p = spec;
    while (*p) {
        while (*p == ' ' || *p == ';' || *p == '\n' || *p == '\t') ++p;
        if (!*p) break;
        const char* eq = std::strchr(p, '=');
        if (!eq) break;
        std::string key(p, eq);
        char* end = nullptr;
        double v = std::strtod(eq + 1, &end);
        int idx = t.find(key);
        if (idx < 0) ++unknown;
        else {
            const ParamDef& d = t.defs[(size_t)idx];
            values[idx] = (float)(v < d.minV ? d.minV : (v > d.maxV ? d.maxV : v));
        }
        p = end;
    }
    return unknown;
}

std::string kitToText(const float* values)
{
    const ParamTable& t = paramTable();
    std::string out = "DrumForge kit v1\n";
    char b[96];
    for (int i = 0; i < t.size(); ++i) {
        if (t.isSession(i)) continue;
        std::snprintf(b, sizeof b, "%s=%.6g\n", t.defs[(size_t)i].key.c_str(), (double)values[i]);
        out += b;
    }
    return out;
}

int kitFromText(const std::string& text, float* values)
{
    const ParamTable& t = paramTable();
    if (text.compare(0, 16, "DrumForge kit v1") != 0) return -1;
    int n = 0;
    size_t pos = text.find('\n');
    while (pos != std::string::npos && pos + 1 < text.size()) {
        const size_t start = pos + 1;
        size_t end = text.find('\n', start);
        std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        pos = end;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const int idx = t.find(line.substr(0, eq));
        if (idx < 0 || t.isSession(idx)) continue;
        char* e = nullptr;
        const double v = std::strtod(line.c_str() + eq + 1, &e);
        if (e == line.c_str() + eq + 1 || v != v) continue;
        const ParamDef& d = t.defs[(size_t)idx];
        values[idx] = (float)(v < d.minV ? d.minV : (v > d.maxV ? d.maxV : v));
        ++n;
    }
    return n;
}

void presetApply(int index, float* values)
{
    const ParamTable& t = paramTable();
    for (int i = 0; i < t.size(); ++i) values[i] = t.defs[(size_t)i].defV;
    presetParse(preset(index).spec, values);
}

}  // namespace df
