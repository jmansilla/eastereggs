# Generated by Django 5.0.7 on 2024-08-08 23:04

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0005_rename_group_number_sogroup_repo_name_and_more'),
    ]

    operations = [
        migrations.AlterField(
            model_name='sogroup',
            name='repo_name',
            field=models.CharField(help_text='The repo name must follow the pattern soYYYYlabZgXX where YYYY is current year, Z is the Lab number, and XX is the actual group number (it may start with 0)', max_length=10, unique=True),
        ),
    ]
