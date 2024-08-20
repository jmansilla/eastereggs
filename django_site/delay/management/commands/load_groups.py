import csv
from django.core.management.base import BaseCommand
from delay.models import SOGroup


class Command(BaseCommand):
    help = 'Load SOGroup instances from a CSV file'

    def add_arguments(self, parser):
        parser.add_argument('csv_filepath', type=str, help='The CSV file to load data from')

    def handle(self, *args, **kwargs):
        csv_filepath = kwargs['csv_filepath']
        reader = csv.DictReader(open(csv_filepath))
        for row in reader:
            _, created = SOGroup.objects.get_or_create(
                repo_name=row['repo_name'],
            )
            if not created:
                self.stdout.write(
                    self.style.ERROR(f'Could not create group {row["repo_name"]}')
                )
