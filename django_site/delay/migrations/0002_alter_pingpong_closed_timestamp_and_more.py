# Generated by Django 5.0.7 on 2024-07-31 22:27

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0001_initial'),
    ]

    operations = [
        migrations.AlterField(
            model_name='pingpong',
            name='closed_timestamp',
            field=models.DateTimeField(blank=True, default=None, help_text="Leave Empty. It'll be filled when the ping was closed", null=True),
        ),
        migrations.AlterField(
            model_name='sogroup',
            name='beaten_timestamp',
            field=models.DateTimeField(blank=True, default=None, help_text="Leave Empty. It'll be filled when the group defeats the delay", null=True),
        ),
    ]
