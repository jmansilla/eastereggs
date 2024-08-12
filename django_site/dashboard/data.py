import json
import plotly.express as px

from dashboards.component.table import TableSerializer
from delay.models import SOGroup
from django.db.models import Count


class DashboardData:
    @staticmethod
    def groups_by_challenge_won(**kwargs):
        groups = SOGroup.objects.values('challenge_won').annotate(count=Count('challenge_won'))
        data = dict(
            challenge_won=[group['challenge_won'] for group in groups],
            count=[group['count'] for group in groups]
        )
        print(data)

        fig = px.pie(
            data,
            names='challenge_won',
            values='count',
        )
        print(fig)

        return fig.to_json()



class GroupTableSerializer(TableSerializer):
    class Meta:
        columns = {
            "group_number": "Group Number",
            "challenge_won": "Has beeten the bug?"
        }
        model = SOGroup
