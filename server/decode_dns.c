#include "decode_dns.h"

char *extract_dns_name(void *, char *);
char dnsname[NS_MAXDNAME];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!
 * @brief decode_dns accepts a pointer to the start of the DNS header and returns a pointer to a character string that is the resolved IP address
 *
 * 		See RFC 1035 section 4 for details on DNS message formats
 *
 * @param dns pointer to the DNS response starting with the header
 * @return a pointer the the resolved IP address (Note: This returns a pointer to allocated memory that must be freed after use.
 */
char *decode_dns(void *dns)
{
	void *dnsdata = NULL;			// Pointer to start of DNS data
	DNS_hdr *dnshdr = NULL;			// Pointer to start of DNS header
	char *resolved_address;

	dnshdr = (DNS_hdr *) dns;
	dnsdata = (void *)dnshdr + sizeof(DNS_hdr);	// Pointer to start of DNS data
	if ( DNS_QR(dnshdr)) {	// Determine if it's a reply and, if so, decode it.
		char *dnsname;
		int i, type;
		int q_records = ntohs(dnshdr->qdcount);
		int a_records = ntohs(dnshdr->ancount);

		// Process DNS Query Records
		for (i = 0; i < q_records; i++) {			// Process q_records query records
			dnsname = dnsdata;
			if (*((int8_t *)dnsdata) > 0) 			// Determine if dnsdata points to an encoded name or to a pointer
				dnsdata += strlen(dnsname) + 1;		// Skip over the name
			else									// Otherwise, skip over the pointer
				dnsdata += 2;						// dnsdata now points to query type
			type = DNS_type(dnsdata);

			switch(type) {
				case ns_t_a:		// IPv4 Address records
					DLX(4, printf("\t DNS Response\tQ: %s\n", extract_dns_name(dns, dnsname)));
					break;

#if 0			// No support for IPv6
				case ns_t_aaaa:		// IPv6 AAAA queries
					DLX(6, printf("\t DNS Response\tIPv6 Q: %s\n", extract_dns_name(dns, dnsname)));
					break;
#endif
				default:	//Return without action for all other record types
					return NULL;
			}
			dnsdata += sizeof(ns_query_typeclass);		// Reposition dnsdata
		}
		if ( DNS_RCODE(dnshdr) ) {
			switch (DNS_RCODE(dnshdr)) {
				case 1:	DLX(4, printf("\t\t\tERROR: Name server unable to interpret query.\n")); break;
				case 2:	DLX(4, printf("\t\t\tERROR: Name server failure\n")); break;
				case 3:	if (DNS_AA(dnshdr)) DLX(6, printf("\t\t\tA: No such name\n")); break;
				case 5:	DLX(4, printf("\t\t\tERROR: Name server refused request.\n")); break;

				default:
					DLX(6, printf("\t\t\tERROR: Undefined response code %i returned.\n", (uint8_t)DNS_RCODE(dnshdr))); break;
			}
		}
#if 0
		if ( DNS_ZERO(dnshdr) )		// Just checking...
			DLX(6, printf("\t\t*** ALERT *** Normally zero reserved DNS header field contains data: %i\n", DNS_ZERO(dnshdr)));
#endif

		// Process DNS Answer Records
		for (i = 0; i < a_records; i++) {			// Process a_records answer records
			DNS_rr_data *rr;
			void *rdata;
			struct in_addr ipaddr;
//			struct in6_addr ip6addr;
			char addrbuf[INET6_ADDRSTRLEN] = {0};

			dnsname = dnsdata;
			if (*((int8_t *)dnsdata) > 0) 			// Determine if dnsdata points to an encoded name or to a pointer
				dnsdata += strlen(dnsname) + 1;		// Skip over the name
			else									// Otherwise, skip over the pointer
				dnsdata += 2;						// dnsdata now points to query type
			type = ntohs(((ns_query_typeclass *)dnsdata)->type);
			rr = (DNS_rr_data *) dnsdata;
			rdata = (void *) rr + sizeof(struct _dns_rr_data);
			switch (type) {
				case ns_t_a:	// IPv4 Address Record
					memcpy(&ipaddr, rdata, ntohs(rr->rdlength));
					if (inet_ntop(AF_INET, (const void *)&ipaddr, addrbuf, INET6_ADDRSTRLEN) == NULL)
						perror("IPv4 address decode error");
					else
						DLX(6, printf("\t\t\t     A: %s\tTTL: %i\n", addrbuf, ntohl(rr->ttl)));
						if ((resolved_address = (char *)calloc(strlen(addrbuf)+1, 1)) == NULL) {
							DLX(4, printf("ERROR: calloc failed\n"));
						}
						strcpy(resolved_address, addrbuf);
						DLX(4, printf("\tResolved Address: %s\n", resolved_address));
						return resolved_address;
					break;

#if 0			// No support for IPv6
				case ns_t_aaaa:	// IPv6 Address Record
					memcpy(&ip6addr, rdata, ntohs(rr->rdlength));
					if (inet_ntop(AF_INET6, (const void *)&ip6addr, addrbuf, INET6_ADDRSTRLEN) == NULL)
						perror("IPv6 address decode error");
					else
						DLX(6, printf("\t\t\t     A: %s\tTTL: %i\n", addrbuf, ntohl(rr->ttl)));
					if ((resolved_address = (char *)malloc(strlen(addrbuf)+1)) == NULL) {
						DLX(4, printf("ERROR: malloc failed\n"));
					}
					strcpy(resolved_address, addrbuf);
					return resolved_address;
					break;
#endif
				default:	// Other record types not handled
					DLX(4, printf("\t\t\tResource record data of type %d not decoded.\n", type));
					break;

			}
			dnsdata = rdata + ntohs(rr->rdlength);		// Reposition dnsdata pointer for next record
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 	dnsname decodes the DNS name pointed to by namep and returns it in staticly allocated dnsname.
// 	Note that this name gets overwritten for each call.
/*!
 * @brief Decode
 * @param dns
 * @param namep
 * @return
 */
char *extract_dns_name(void *dns, char *namep) {
	char *p;
	int8_t len;
	uint16_t ptr;

	p = dnsname;
	len = (int8_t) *namep;						// Use first byte as length
	while (len) {							// If 0, then stop
		if (len < 0) {						// If length is negative, then it's a pointer
			ptr = ntohs(*((uint16_t *) namep)) & 0x3FFF;	// Compute pointer as an offset into the DNS record
			namep = dns + ptr;
			len = (int8_t) *namep;
			continue;
		}
		strncpy(p, ++namep, len);
		p += len;
		*p++ = '.';
		namep += len;
		len = (int8_t) *namep;
	}
	*--p='\0';
	return dnsname;
}
