# Generated by Django 5.0.7 on 2024-08-09 00:42

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('delay', '0008_alter_sogroup_repo_name'),
    ]

    operations = [
        migrations.RenameField(
            model_name='sogroup',
            old_name='secret_key',
            new_name='password_to_win',
        ),
    ]
