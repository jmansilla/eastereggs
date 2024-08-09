import pandas

from django.core.management.base import BaseCommand
from delay.models import SOGroup


class Command(BaseCommand):
    help = 'Load SOGroup instances from a CSV file'

    def add_arguments(self, parser):
        parser.add_argument('csv_filepath', type=str, help='The CSV file to load data from')

    def handle(self, *args, **kwargs):
        csv_filepath = kwargs['csv_filepath']
        groups = pandas.read_csv(csv_filepath, dtype={'group_number': object})

        for _, row in groups.iterrows():
            _, created = SOGroup.objects.get_or_create(
                group_number=row['group_number'],
                group_name=row['group_name'],
                secret_key=row['secret_key']
            )
            if not created:
                self.stdout.write(
                    self.style.ERROR(f'Could not create group {row['group_name']}')
                )
