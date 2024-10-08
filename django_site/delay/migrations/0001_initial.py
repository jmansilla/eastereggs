# Generated by Django 5.0.7 on 2024-07-31 00:21

import django.db.models.deletion
from django.db import migrations, models


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='SOGroup',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('group_number', models.CharField(help_text='The group number is intended to be just a number (may start with 0)', max_length=10, unique=True)),
                ('group_name', models.CharField(max_length=100)),
                ('secret_key', models.CharField(max_length=100)),
                ('current_delay', models.IntegerField(default=0, help_text='Current delay in milliseconds')),
                ('delay_increment', models.IntegerField(default=25, help_text='Delay increment in milliseconds')),
                ('delay_roof', models.IntegerField(default=10000, help_text='Maximum delay in milliseconds')),
                ('beaten', models.BooleanField(default=False, help_text='True if the group has been beaten the delay')),
                ('beaten_timestamp', models.DateTimeField(blank=True, default=None, help_text="Leave Empty. It'll be filled when the group defeats the delay")),
                ('wasted_time', models.IntegerField(default=0, help_text='Total wasted time accumulated in minutes (rounded)')),
            ],
        ),
        migrations.CreateModel(
            name='PingPong',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('timestamp', models.DateTimeField(auto_now_add=True)),
                ('delay_recommended', models.IntegerField(default=0)),
                ('user_id', models.CharField(help_text='User-Id of whom initiated the ping', max_length=100)),
                ('closed', models.BooleanField(default=False, help_text='True if the ping was closed (ie, it awaited the delay and closed it)')),
                ('closed_timestamp', models.DateTimeField(blank=True, default=None, help_text="Leave Empty. It'll be filled when the ping was closed")),
                ('tampering_attemp', models.BooleanField(default=False, help_text='True if the ping was tampered with wrong key')),
                ('group', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='delay.sogroup')),
            ],
        ),
    ]
