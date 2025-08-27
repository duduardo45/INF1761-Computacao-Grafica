class Quad;
using QuadPtr = std::shared_ptr<Quad>;
#ifndef QUAD_H
#define QUAD_H
#include "shape.h"
  class Quad : public Shape{
    unsigned int m_vao;
    protected:
      Quad();//constructor:createbufferobjects
      //andinitilizetheVAOobject
    public:
      staticQuadPtrMake();//factory:callsconstructorandreturns
      //thesharedpointer
      virtual~Quad();//deletebufferobjects
      virtualvoidDraw()const;//callsdrawcomand
  };
#endif