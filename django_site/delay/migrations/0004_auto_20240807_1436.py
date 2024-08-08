# Generated by Django 5.0.7 on 2024-08-07 16:22

from django.db import migrations

datetime_stamp_show = '2024-08-27 00:00:00 -0300'

show_txt = """Dentro del código hay oculto un **compartimiento anómalo**, que además probablemente con el paso del tiempo, empiece a resultar cada vez más molesto para ustedes.

Al ser este un punto-estrella, no hay ninguna obligación de resolver este punto para aprobar el lab, pero sí se considerará como mérito extra el haberlo resuelto.

Si ya estuvieron trabajando en el lab, puede que ya hayan notado el mencionado comportamiento anómalo. Si incluso avanzaron tanto como para descubrir la causa, e incluso evitar su molesto efecto, es momento de que ahora presten atención a realizar los pasos necesarios para reclamar su merecido premio.

### Pasos a seguir

1. Prestar atención a si el comportamiento del Lab que están construyendo es tal como ustedes esperan que sea
2. Si aparece algo "extraño" (resultados inesperados, archivos que se modifican repentinamente, demoras inexplicables, etc), buscar qué puede estar causándolo.
3. Si encuentran efectivamente el causante de esto (y no es un bug de ustedes), solucionenlo. Prestar delicada atención a cómo lo solucionan pues...
    1. Hay sólo una forma de solucionarlo y que la cátedra se entere inmediatamente de que lo han solucionado.
    2. Cuando lo solucionen bien, nos enteraremos sin que nos escriban un mail, ni por zulip, ni nada de eso. Con tocar de manera exacta el código culpable, sin eliminarlo, nos enteraremos.
    3. Esto es una carrera. Quienes antes lo encuentren, quedarán mejor rankeados
    4. Idealmente, lo mejor es que no le avisen a los otros grupos. No es algo que podamos impedir. Sí es algo que muy posiblemente podamos descubrir. Pero además, que ustedes les avisen a sus compañeros atenta *de lleno* con los **objetivos** de esta actividad, y los grupos avisados pierden más de lo que ganan(leer más abajo sobre esto)


### Objetivos

Más allá del punto de vista lúdico y/o competitivo de esta actividad, el principal objetivo tiene que ver con exponerles a una de las actividades fundamentales en la profesión: **aprender a descubrir y solucionar problemas**.

[Wikipedia](https://es.wikipedia.org/wiki/Depuraci%C3%B3n_de_programas) dice
> Si bien existen técnicas para la revisión sistemática del código fuente y se cuenta con medios computacionales para la detección de errores (depuradores) y facilidades integradas en distintos sistemas y ambientes de desarrollo integrado (IDEs), **sigue siendo en buena medida una actividad manual, que desafía la paciencia, la imaginación y la intuición de programadores**.

Aprender a desarrollar software es en buena medida, también aprender a corregir comportamientos inesperados.

* Darse cuenta de que están sucediendo comportamientos inesperados es parte de la intuición que es MUY valioso que empiecen a desarrollar. (es por eso que esta parte del enunciado apareció varios días después, para que ojalá ya tengamos grupos que hayan empezado a escarbar el código porque pasan cosas *raras*)
* Ponerse el sombrero de Sherlock Holmes y salir a la caza de tal *bug* es lo que más a menudo les tocará hacer. Experimentémoslo de manera conjunta en este lab."""

print_msgs = [
    ('2024-08-29 12:00:00 -0300', 'print_msg_1', 'Hay un bug en tu código.'),
    ('2024-09-01 12:00:00 -0300', 'print_msg_2', '¿Puede ser que esté cada vez más lento ejecutar el proyecto?'),
    ('2024-09-03 12:00:00 -0300', 'print_msg_3', '¿Te fijaste qué sucede sin estar conectado a internet?'),
    ('2024-09-05 12:00:00 -0300', 'print_msg_4', 'Quizás la contraseña esté en el mismo response donde se declara el delay.'),
]


def forwards_func(apps, schema_editor):
    # We get the model from the versioned app registry;
    # if we directly import it, it'll be the wrong version
    Deadline = apps.get_model("delay", "Deadline")
    db_alias = schema_editor.connection.alias
    bulk = [
        Deadline(name="show_challenge_explanation", deadline=datetime_stamp_show, text=show_txt),
    ] + [
        Deadline(name=name, deadline=stamp, text=txt) for stamp, name, txt in print_msgs
    ]
    Deadline.objects.using(db_alias).bulk_create(bulk)

def reverse_func(apps, schema_editor):
    # forwards_func() creates two Deadline instances,
    # so reverse_func() should delete them.
    Deadline = apps.get_model("delay", "Deadline")
    db_alias = schema_editor.connection.alias
    Deadline.objects.using(db_alias).filter(name="show_challenge_explanation").delete()
    print_names = [name for stamp, name, txt in print_msgs]
    Deadline.objects.using(db_alias).filter(name__in=print_names).delete()


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0003_deadline'),
    ]

    operations = [migrations.RunPython(forwards_func, reverse_func),
    ]
