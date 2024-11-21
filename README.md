# Modelo Earth

Este projeto utiliza OpenGL para renderizar um modelo 3D da Terra. O código carrega um modelo OBJ, aplica uma textura e implementa controle de câmera e iluminação. Ele utiliza as bibliotecas GLFW, GLEW, GLM, Assimp e stb_image para criar um ambiente de renderização 3D.

## Funcionalidades

- **Carregamento de Modelo**: Carrega um modelo OBJ usando a biblioteca Assimp.
- **Texturas**: Aplica uma textura ao modelo 3D utilizando a stb_image.
- **Controle de Câmera**: Navegue pela cena com o teclado e o mouse.
- **Iluminação**: Ajuste a intensidade da luz usando as setas do teclado.
- **Shaders**: Utiliza shaders personalizados para calcular iluminação e aplicar texturas.

---

## Pré-requisitos

Para compilar e executar este projeto, você precisa:

1. **Bibliotecas Necessárias**:
   - [GLFW](https://www.glfw.org/)
   - [GLEW](http://glew.sourceforge.net/)
   - [GLM](https://glm.g-truc.net/)
   - [Assimp](https://github.com/assimp/assimp)
   - [stb_image](https://github.com/nothings/stb)

2. **Compilador**:
   - GCC ou outro compilador com suporte a C++11 ou superior.
    
## Controles

- **W, A, S, D**: Movem a câmera para frente, esquerda, trás e direita, respectivamente.
- **Mouse**: Controla a rotação da câmera.
- **Scroll do Mouse**: Ajusta o campo de visão (zoom).
- **Setas para Cima/Baixo**: Ajustam a intensidade da luz.
