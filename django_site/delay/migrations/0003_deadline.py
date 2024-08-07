# Generated by Django 5.0.7 on 2024-08-07 14:32

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0002_alter_pingpong_closed_timestamp_and_more'),
    ]

    operations = [
        migrations.CreateModel(
            name='Deadline',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=100, unique=True)),
                ('deadline', models.DateTimeField()),
            ],
        ),
    ]
