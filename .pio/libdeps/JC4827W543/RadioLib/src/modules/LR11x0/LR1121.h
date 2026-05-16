#if !defined(_RADIOLIB_LR1121_H)
#define _RADIOLIB_LR1121_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"
#include "LR11x0.h"
#include "LR1120.h"

/*!
  \class LR1121
  \brief Derived class for %LR1121 modules.
*/
class LR1121: public LR1120 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR1121(Module* mod); // cppcheck-suppress noExplicitConstructor

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
