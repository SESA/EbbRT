#include <unordered_map>

#include "arch/args.hpp"
#include "app/app.hpp"
#include "lrt/boot.hpp"
#include "lrt/event.hpp"
#include "lrt/mem.hpp"
#include "lrt/trans_impl.hpp"

void
ebbrt::lrt::trans::init_ebbs()
{
  return;
}

bool
ebbrt::lrt::trans::init(unsigned num_cores)
{
  return true;
}


void
ebbrt::lrt::trans::init_cpu()
{
  return;
}

bool
ebbrt::lrt::trans::_trans_precall(ebbrt::Args* args,
                                  ptrdiff_t fnum,
                                  FuncRet* fret)
{
  return true;
}

void*
ebbrt::lrt::trans::_trans_postcall(void* ret)
{
  return NULL; 
}

bool
ebbrt::lrt::trans::InitRoot::PreCall(ebbrt::Args* args,
                                     ptrdiff_t fnum,
                                     FuncRet* fret,
                                     EbbId id)
{
  return true;
}

void*
ebbrt::lrt::trans::InitRoot::PostCall(void* ret)
{
  return NULL;
}

void
ebbrt::lrt::trans::cache_rep(EbbId id, EbbRep* rep)
{
  return;
}


void default_func0(){}
void default_func1(){}
void default_func2(){}
void default_func3(){}
void default_func4(){}
void default_func5(){}
void default_func6(){}
void default_func7(){}
void default_func8(){}
void default_func9(){}

void default_func10(){}
void default_func11(){}
void default_func12(){}
void default_func13(){}
void default_func14(){}
void default_func15(){}
void default_func16(){}
void default_func17(){}
void default_func18(){}
void default_func19(){}

void default_func20(){}
void default_func21(){}
void default_func22(){}
void default_func23(){}
void default_func24(){}
void default_func25(){}
void default_func26(){}
void default_func27(){}
void default_func28(){}
void default_func29(){}

void default_func30(){}
void default_func31(){}
void default_func32(){}
void default_func33(){}
void default_func34(){}
void default_func35(){}
void default_func36(){}
void default_func37(){}
void default_func38(){}
void default_func39(){}

void default_func40(){}
void default_func41(){}
void default_func42(){}
void default_func43(){}
void default_func44(){}
void default_func45(){}
void default_func46(){}
void default_func47(){}
void default_func48(){}
void default_func49(){}

void default_func50(){}
void default_func51(){}
void default_func52(){}
void default_func53(){}
void default_func54(){}
void default_func55(){}
void default_func56(){}
void default_func57(){}
void default_func58(){}
void default_func59(){}

void default_func60(){}
void default_func61(){}
void default_func62(){}
void default_func63(){}
void default_func64(){}
void default_func65(){}
void default_func66(){}
void default_func67(){}
void default_func68(){}
void default_func69(){}

void default_func70(){}
void default_func71(){}
void default_func72(){}
void default_func73(){}
void default_func74(){}
void default_func75(){}
void default_func76(){}
void default_func77(){}
void default_func78(){}
void default_func79(){}

void default_func80(){}
void default_func81(){}
void default_func82(){}
void default_func83(){}
void default_func84(){}
void default_func85(){}
void default_func86(){}
void default_func87(){}
void default_func88(){}
void default_func89(){}

void default_func90(){}
void default_func91(){}
void default_func92(){}
void default_func93(){}
void default_func94(){}
void default_func95(){}
void default_func96(){}
void default_func97(){}
void default_func98(){}
void default_func99(){}

void default_func100(){}
void default_func101(){}
void default_func102(){}
void default_func103(){}
void default_func104(){}
void default_func105(){}
void default_func106(){}
void default_func107(){}
void default_func108(){}
void default_func109(){}

void default_func110(){}
void default_func111(){}
void default_func112(){}
void default_func113(){}
void default_func114(){}
void default_func115(){}
void default_func116(){}
void default_func117(){}
void default_func118(){}
void default_func119(){}

void default_func120(){}
void default_func121(){}
void default_func122(){}
void default_func123(){}
void default_func124(){}
void default_func125(){}
void default_func126(){}
void default_func127(){}
void default_func128(){}
void default_func129(){}

void default_func130(){}
void default_func131(){}
void default_func132(){}
void default_func133(){}
void default_func134(){}
void default_func135(){}
void default_func136(){}
void default_func137(){}
void default_func138(){}
void default_func139(){}

void default_func140(){}
void default_func141(){}
void default_func142(){}
void default_func143(){}
void default_func144(){}
void default_func145(){}
void default_func146(){}
void default_func147(){}
void default_func148(){}
void default_func149(){}

void default_func150(){}
void default_func151(){}
void default_func152(){}
void default_func153(){}
void default_func154(){}
void default_func155(){}
void default_func156(){}
void default_func157(){}
void default_func158(){}
void default_func159(){}

void default_func160(){}
void default_func161(){}
void default_func162(){}
void default_func163(){}
void default_func164(){}
void default_func165(){}
void default_func166(){}
void default_func167(){}
void default_func168(){}
void default_func169(){}

void default_func170(){}
void default_func171(){}
void default_func172(){}
void default_func173(){}
void default_func174(){}
void default_func175(){}
void default_func176(){}
void default_func177(){}
void default_func178(){}
void default_func179(){}

void default_func180(){}
void default_func181(){}
void default_func182(){}
void default_func183(){}
void default_func184(){}
void default_func185(){}
void default_func186(){}
void default_func187(){}
void default_func188(){}
void default_func189(){}

void default_func190(){}
void default_func191(){}
void default_func192(){}
void default_func193(){}
void default_func194(){}
void default_func195(){}
void default_func196(){}
void default_func197(){}
void default_func198(){}
void default_func199(){}

void default_func200(){}
void default_func201(){}
void default_func202(){}
void default_func203(){}
void default_func204(){}
void default_func205(){}
void default_func206(){}
void default_func207(){}
void default_func208(){}
void default_func209(){}

void default_func210(){}
void default_func211(){}
void default_func212(){}
void default_func213(){}
void default_func214(){}
void default_func215(){}
void default_func216(){}
void default_func217(){}
void default_func218(){}
void default_func219(){}

void default_func220(){}
void default_func221(){}
void default_func222(){}
void default_func223(){}
void default_func224(){}
void default_func225(){}
void default_func226(){}
void default_func227(){}
void default_func228(){}
void default_func229(){}

void default_func230(){}
void default_func231(){}
void default_func232(){}
void default_func233(){}
void default_func234(){}
void default_func235(){}
void default_func236(){}
void default_func237(){}
void default_func238(){}
void default_func239(){}

void default_func240(){}
void default_func241(){}
void default_func242(){}
void default_func243(){}
void default_func244(){}
void default_func245(){}
void default_func246(){}
void default_func247(){}
void default_func248(){}
void default_func249(){}

void default_func250(){}
void default_func251(){}
void default_func252(){}
void default_func253(){}
void default_func254(){}
void default_func255(){}

void (*ebbrt::lrt::trans::default_vtable[256])() = {
  default_func0,
  default_func1,
  default_func2,
  default_func3,
  default_func4,
  default_func5,
  default_func6,
  default_func7,
  default_func8,
  default_func9,
  default_func10,
  default_func11,
  default_func12,
  default_func13,
  default_func14,
  default_func15,
  default_func16,
  default_func17,
  default_func18,
  default_func19,
  default_func20,
  default_func21,
  default_func22,
  default_func23,
  default_func24,
  default_func25,
  default_func26,
  default_func27,
  default_func28,
  default_func29,
  default_func30,
  default_func31,
  default_func32,
  default_func33,
  default_func34,
  default_func35,
  default_func36,
  default_func37,
  default_func38,
  default_func39,
  default_func40,
  default_func41,
  default_func42,
  default_func43,
  default_func44,
  default_func45,
  default_func46,
  default_func47,
  default_func48,
  default_func49,
  default_func50,
  default_func51,
  default_func52,
  default_func53,
  default_func54,
  default_func55,
  default_func56,
  default_func57,
  default_func58,
  default_func59,
  default_func60,
  default_func61,
  default_func62,
  default_func63,
  default_func64,
  default_func65,
  default_func66,
  default_func67,
  default_func68,
  default_func69,
  default_func70,
  default_func71,
  default_func72,
  default_func73,
  default_func74,
  default_func75,
  default_func76,
  default_func77,
  default_func78,
  default_func79,
  default_func80,
  default_func81,
  default_func82,
  default_func83,
  default_func84,
  default_func85,
  default_func86,
  default_func87,
  default_func88,
  default_func89,
  default_func90,
  default_func91,
  default_func92,
  default_func93,
  default_func94,
  default_func95,
  default_func96,
  default_func97,
  default_func98,
  default_func99,
  default_func100,
  default_func101,
  default_func102,
  default_func103,
  default_func104,
  default_func105,
  default_func106,
  default_func107,
  default_func108,
  default_func109,
  default_func110,
  default_func111,
  default_func112,
  default_func113,
  default_func114,
  default_func115,
  default_func116,
  default_func117,
  default_func118,
  default_func119,
  default_func120,
  default_func121,
  default_func122,
  default_func123,
  default_func124,
  default_func125,
  default_func126,
  default_func127,
  default_func128,
  default_func129,
  default_func130,
  default_func131,
  default_func132,
  default_func133,
  default_func134,
  default_func135,
  default_func136,
  default_func137,
  default_func138,
  default_func139,
  default_func140,
  default_func141,
  default_func142,
  default_func143,
  default_func144,
  default_func145,
  default_func146,
  default_func147,
  default_func148,
  default_func149,
  default_func150,
  default_func151,
  default_func152,
  default_func153,
  default_func154,
  default_func155,
  default_func156,
  default_func157,
  default_func158,
  default_func159,
  default_func160,
  default_func161,
  default_func162,
  default_func163,
  default_func164,
  default_func165,
  default_func166,
  default_func167,
  default_func168,
  default_func169,
  default_func170,
  default_func171,
  default_func172,
  default_func173,
  default_func174,
  default_func175,
  default_func176,
  default_func177,
  default_func178,
  default_func179,
  default_func180,
  default_func181,
  default_func182,
  default_func183,
  default_func184,
  default_func185,
  default_func186,
  default_func187,
  default_func188,
  default_func189,
  default_func190,
  default_func191,
  default_func192,
  default_func193,
  default_func194,
  default_func195,
  default_func196,
  default_func197,
  default_func198,
  default_func199,
  default_func200,
  default_func201,
  default_func202,
  default_func203,
  default_func204,
  default_func205,
  default_func206,
  default_func207,
  default_func208,
  default_func209,
  default_func210,
  default_func211,
  default_func212,
  default_func213,
  default_func214,
  default_func215,
  default_func216,
  default_func217,
  default_func218,
  default_func219,
  default_func220,
  default_func221,
  default_func222,
  default_func223,
  default_func224,
  default_func225,
  default_func226,
  default_func227,
  default_func228,
  default_func229,
  default_func230,
  default_func231,
  default_func232,
  default_func233,
  default_func234,
  default_func235,
  default_func236,
  default_func237,
  default_func238,
  default_func239,
  default_func240,
  default_func241,
  default_func242,
  default_func243,
  default_func244,
  default_func245,
  default_func246,
  default_func247,
  default_func248,
  default_func249,
  default_func250,
  default_func251,
  default_func252,
  default_func253,
  default_func254,
  default_func255
};
