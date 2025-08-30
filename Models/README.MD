
# Impresion 3D del auto
Para el chasis del auto elegí el chasis OpenRC de Duke Doks. El modelo es genial, solo requiere algunos tornillos M3 y tuercas para su ensamble. 
Lo imprimi en mi Ender 3 V2 usando PETG (Grillon3) para la mayoria de las partes y Simpliflex (Grillon3) para las partes flexibles:
- Link del modelo: https://www.thingiverse.com/thing:4892947
- Guia de ensamble: https://dukedoks.com/portfolio/guia-chasis-rc/

Las partes electrónicas NO son las del modelo, fueron reemplazadas por partes que pude conseguir en Argentina a un precio accesible (o recicladas) y adaptadas para utilizar el microcontrolador como "cerebro" del auto.

## Modificaciones del Modelos

Modifique algunas partes del modelo para adaptarla a los motores que decidi utilizar (2 Motores DC Pololu de 500 RPM). En particular modifique:
- "Rear_Frame_L.stl" para añadir las guias para el motor 
- "Gear_1.stl" para adaptarlo al eje del motor DC
- Cree el modelo "motorHoldPololu" para poder instalar los motores en las guias de la parte trasera
- Genere unos anillos para reemplazar los rulemanes de las ruedas (ya que no los pude conseguir). "fake_bear_back" y "fake_bear_front"

Al utilizar 2 motores traseros, sera necesario imprimir:
- 2x gearModPololu.stl
- 2x Gear_2.stl
- 2x Popolu_Adapter.stl

Si no conseguis los rulemanes tambien sera necesario imprimir los siguientes modelos para reemplazarlos:
- 2x "fake_bear_back.stl"
- 2x "fake_bear_front.stl"

## Guia de Ensamble

El resto del ensamble es tal cual lo indica la guiá del autor del modelo: https://dukedoks.com/portfolio/guia-chasis-rc/
