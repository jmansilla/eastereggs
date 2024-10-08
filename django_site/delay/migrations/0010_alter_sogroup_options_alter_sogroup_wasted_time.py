# Generated by Django 5.0.7 on 2024-08-09 01:20

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0009_rename_secret_key_sogroup_password_to_win'),
    ]

    operations = [
        migrations.AlterModelOptions(
            name='sogroup',
            options={'verbose_name': 'S.O.LabGroup', 'verbose_name_plural': 'S.O.LabGroups'},
        ),
        migrations.AlterField(
            model_name='sogroup',
            name='wasted_time',
            field=models.IntegerField(default=0, editable=False, help_text='Total wasted time accumulated in milliseconds'),
        ),
    ]
