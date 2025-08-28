class Shape;
using ShapePtr = std::shared_ptr<Shape>;
#ifndef SHAPE_H
#define SHAPE_H
  class Shape{
    protected:
      auto mode = GL_TRIANGLES;
      unsigned int nverts;
      auto type = GL_UNSIGNED_INT;
      unsigned int offset = 0;
      Shape(){}
    public:
      virtual ~Shape(){}
      virtual void Draw()=0;
  };
#endif