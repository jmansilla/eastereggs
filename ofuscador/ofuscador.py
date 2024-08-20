import re

def string2ascii(cadena):
    cadena = cadena[1:-1]
    # Reemplazar secuencias de escape manualmente
    cadena = cadena.replace("\\033","\033")
    cadena = re.sub(r'\\([ntr0])', lambda m: {'n': '\n', 't': '\t', 'r': '\r', '0': '\0'}[m.group(1)], cadena)

    result = ""
    for i in cadena:
        result += hex(ord(i)).replace('0x', '\\x')
    
    return f'"{result}"'
    

def procesar_archivo(archivo):
    # Leer el contenido del archivo
    with open(archivo, 'r') as f:
        contenido = f.read()

    # Expresión regular para capturar solo cadenas literales en C (comillas dobles)
    regex = r'"(?:\\.|[^"\\])*"'

    # Función para aplicar la transformación a las coincidencias
    def reemplazar(m):
        cadena_original = m.group(0)
        cadena_transformada = string2ascii(cadena_original)
        return cadena_transformada

    # Reemplazar todas las cadenas literales usando la función de transformación
    contenido_modificado = re.sub(regex, reemplazar, contenido)

    # Imprimir el contenido modificado (puedes cambiar esto para escribir en un archivo si lo deseas)
    print(contenido_modificado)

# Nombre del archivo C
nombre_archivo = '../c_module/pingpong.c'

# Procesar el archivo
procesar_archivo(nombre_archivo)