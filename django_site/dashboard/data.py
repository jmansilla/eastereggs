import json
import plotly.express as px

from dashboards.component.table import TableSerializer
from delay.models import SOGroup
from django.db.models import Count


class DashboardData:
    @staticmethod
    def groups_by_beaten(**kwargs):
        groups = SOGroup.objects.values('beaten').annotate(count=Count('beaten'))
        data = dict(
            beaten=[group['beaten'] for group in groups],
            count=[group['count'] for group in groups]
        )

        fig = px.pie(
            data,
            names='beaten',
            values='count',
        )

        return fig.to_json()



class GroupTableSerializer(TableSerializer):
    class Meta:
        columns = {
            "group_number": "Group Number",
            "beaten": "Has beeten the bug?"
        }
        model = SOGroup
