#ifndef SHAPE_H
#define SHAPE_H

#include <memory>

class Shape;
using ShapePtr = std::shared_ptr<Shape>;

  class Shape{
    protected:
      unsigned int mode = GL_TRIANGLES;
      unsigned int nverts;
      unsigned int type = GL_UNSIGNED_INT;
      unsigned int offset = 0;
      Shape(){}
    public:
      virtual ~Shape(){}
      virtual void Draw() const = 0;
  };
#endif